/*
 *  This code was originally from:
 *   UBL, The Universal Talkware Boot Loader
 *    Copyright (C) 2000 Universal Talkware Inc.
 *
 *  However after severe edits, even its own Mum wouldn't recognize it now
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 *  Original from http://www.talkware.net/GPL/UBL
 *
 *   2002-08-25 andy@warmcat.com  threshed around to work with xbox
 *   2002-08-26 andy@warmcat.com  more edits to call speedbump's code to unlock HDD
 *   2002-09-08 andy@warmcat.com  changed to standardized symbol format; ATAPI packet code
 */

#include  "boot.h"
#include "string.h"

////////////////////////////////////
// IDE types and constants

typedef enum {
	EDT_UNKNOWN= 0,
	EDT_XBOXFS
} enumDriveType;

typedef struct {  // this is the retained knowledge about an IDE device after init
    unsigned short m_fwPortBase;
    unsigned short m_wCountHeads;
    unsigned short m_wCountCylinders;
    unsigned short m_wCountSectorsPerTrack;
    unsigned long m_dwCountSectorsTotal; /* total */
    unsigned char m_bLbaMode;	/* am i lba (0x40) or chs (0x00) */
    unsigned char m_szIdentityModelNumber[41];
    unsigned char m_szSerial[21];
		unsigned char m_fDriveExists;
		unsigned char m_fAtapi;  // true if a CDROM, etc
		enumDriveType m_enumDriveType;
} tsHarddiskInfo;

#define IDE_SECTOR_SIZE 0x200
#define IDE_BASE1             (0x1F0u) /* primary controller */

#define IDE_REG_EXTENDED_OFFSET   (0x200u)

#define IDE_REG_DATA(base)          ((base) + 0u) /* word register */
#define IDE_REG_ERROR(base)         ((base) + 1u)
#define IDE_REG_SECTOR_COUNT(base)  ((base) + 2u)
#define IDE_REG_SECTOR_NUMBER(base) ((base) + 3u)
#define IDE_REG_CYLINDER_LSB(base)  ((base) + 4u)
#define IDE_REG_CYLINDER_MSB(base)  ((base) + 5u)
#define IDE_REG_DRIVEHEAD(base)     ((base) + 6u)
#define IDE_REG_STATUS(base)        ((base) + 7u)
#define IDE_REG_COMMAND(base)       ((base) + 7u)
#define IDE_REG_ALTSTATUS(base)     ((base) + IDE_REG_EXTENDED_OFFSET + 6u)
#define IDE_REG_CONTROL(base)       ((base) + IDE_REG_EXTENDED_OFFSET + 6u)
#define IDE_REG_ADDRESS(base)       ((base) + IDE_REG_EXTENDED_OFFSET + 7u)

typedef struct {
    unsigned char m_bPrecomp;
    unsigned char m_bCountSector;
    unsigned char m_bSector;
    unsigned short m_wCylinder;
    unsigned char m_bDrivehead;
#       define IDE_DH_DEFAULT (0xA0)
#       define IDE_DH_HEAD(x) ((x) & 0x0F)
#       define IDE_DH_MASTER  (0x00)
#       define IDE_DH_SLAVE   (0x10)
#       define IDE_DH_DRIVE(x) ((((x) & 1) != 0)?IDE_DH_SLAVE:IDE_DH_MASTER)
#       define IDE_DH_LBA     (0x40)
#       define IDE_DH_CHS     (0x00)

} tsIdeCommandParams;

#define IDE_DEFAULT_COMMAND { 0xFFu, 0x01, 0x00, 0x0000, IDE_DH_DEFAULT | IDE_DH_SLAVE }

typedef enum {
    IDE_CMD_NOOP = 0,
    IDE_CMD_RECALIBRATE = 0x10,
    IDE_CMD_READ_MULTI_RETRY = 0x20,
    IDE_CMD_READ_MULTI = IDE_CMD_READ_MULTI_RETRY,
    IDE_CMD_READ_MULTI_NORETRY = 0x21,

    IDE_CMD_DRIVE_DIAG = 0x90,
    IDE_CMD_SET_PARAMS = 0x91,
    IDE_CMD_STANDBY_IMMEDIATE = 0x94, /* 2 byte command- also send
                                         IDE_CMD_STANDBY_IMMEDIATE2 */
    IDE_CMD_SET_MULTIMODE = 0xC6,
    IDE_CMD_STANDBY_IMMEDIATE2 = 0xE0,
    IDE_CMD_GET_INFO = 0xEC,

		IDE_CMD_ATAPI_PACKET = 0xa0,

		IDE_CMD_SECURITY_UNLOCK = 0xf2
} ide_command_t;

#define printk_debug bprintf


//////////////////////////////////
//  Statics

tsHarddiskInfo tsaHarddiskInfo[2];  // static struct stores data about attached drives


///////////////////////////////////////////////////////////////////////////////////////////////////
//  Helper routines
//

int Delay() { int i=0, u=0; while(u<100) { i+=u++; } return i; }


int BootIdeWaitNotBusy(unsigned uIoBase)
{
	BYTE b = 0x80;
	Delay();
	while (b & 0x80) b=IoInputByte(IDE_REG_ALTSTATUS(uIoBase));
	return b&1;
}

int BootIdeWaitDataReady(unsigned uIoBase)
{
	WORD i = 0;
	Delay();
	do {
		if ( ((IoInputByte(IDE_REG_ALTSTATUS(uIoBase)) & 0x88) == 0x08)	)	{
	    if(IoInputByte(IDE_REG_ALTSTATUS(uIoBase)) & 0x01) return 2;
			return 0;
		}
		i++;
	} while (i != 0);

	if(IoInputByte(IDE_REG_ALTSTATUS(uIoBase)) & 0x01) return 2;
	return 1;
}

int BootIdeIssueAtaCommand(
	unsigned uIoBase,
	ide_command_t command,
	tsIdeCommandParams * params)
{
	IoOutputByte(IDE_REG_SECTOR_COUNT(uIoBase), params->m_bCountSector);
	IoOutputByte(IDE_REG_SECTOR_NUMBER(uIoBase), params->m_bSector);
	IoOutputByte(IDE_REG_CYLINDER_LSB(uIoBase), params->m_wCylinder & 0xFF);
	IoOutputByte(IDE_REG_CYLINDER_MSB(uIoBase), (params->m_wCylinder >> 8) /* & 0x03 */);
	IoOutputByte(IDE_REG_DRIVEHEAD(uIoBase), params->m_bDrivehead);

	IoOutputByte(IDE_REG_COMMAND(uIoBase), command);
	Delay();

	if (BootIdeWaitNotBusy(uIoBase))	{
		printk_debug("error on BootIdeIssueAtaCommand wait 3: error %02X\n", IoInputByte(IDE_REG_ERROR(uIoBase)));
		return 1;
	}
	return 0;
}

int BootIdeWriteAtapiData(unsigned uIoBase, void * buf, size_t size)
{
	WORD * ptr = (unsigned short *) buf;
	WORD w;


	BootIdeWaitDataReady(uIoBase);
	Delay();

	w=IoInputByte(IDE_REG_CYLINDER_LSB(uIoBase));
	w|=(IoInputByte(IDE_REG_CYLINDER_MSB(uIoBase)))<<8;

//	bprintf("(bytes count =%04X)\n", w);

	while (size > 1) {

		IoOutputWord(IDE_REG_DATA(uIoBase), *ptr);
		size -= 2;
		ptr++;
	}
		IoInputByte(IDE_REG_STATUS(uIoBase));
	Delay();
	BootIdeWaitNotBusy(uIoBase);
	Delay();

   if(IoInputByte(IDE_REG_ALTSTATUS(uIoBase)) & 0x01) return 2;
	return 0;
}

// issues a block of data ATA-style

int BootIdeWriteData(unsigned uIoBase, void * buf, size_t size)
{
	register unsigned short * ptr = (unsigned short *) buf;

	BootIdeWaitDataReady(uIoBase);
	Delay();

	while (size > 1) {

		IoOutputWord(IDE_REG_DATA(uIoBase), *ptr);
		size -= 2;
		ptr++;
	}
	Delay();
	BootIdeWaitNotBusy(uIoBase);
	Delay();

   if(IoInputByte(IDE_REG_ALTSTATUS(uIoBase)) & 0x01) return 2;
	return 0;
}


int BootIdeIssueAtapiPacketCommandAndPacket(int nDriveIndex, BYTE *pAtapiCommandPacket12Bytes)
{
	tsIdeCommandParams tsicp = IDE_DEFAULT_COMMAND;
	unsigned 	uIoBase = tsaHarddiskInfo[nDriveIndex].m_fwPortBase;

	tsicp.m_bDrivehead = IDE_DH_DEFAULT | IDE_DH_HEAD(0) | IDE_DH_CHS | IDE_DH_DRIVE(nDriveIndex);
	IoOutputByte(IDE_REG_DRIVEHEAD(uIoBase), tsicp.m_bDrivehead);

		tsicp.m_wCylinder=2048;
		BootIdeWaitNotBusy(uIoBase);
		if(BootIdeIssueAtaCommand(uIoBase, IDE_CMD_ATAPI_PACKET, &tsicp)) {
			printk("  Drive %d: BootIdeIssueAtapiPacketCommandAndPacket 1 FAILED, error=%02X\n", nDriveIndex, IoInputByte(IDE_REG_ERROR(uIoBase)));
			return 1;
		}

		if(BootIdeWaitNotBusy(uIoBase)) {
			printk("  Drive %d: BootIdeIssueAtapiPacketCommandAndPacket 2 FAILED, error=%02X\n", nDriveIndex, IoInputByte(IDE_REG_ERROR(uIoBase)));
			return 1;
		}

//					printk("  Drive %d:   status=0x%02X, error=0x%02X\n",
//				nDriveIndex, IoInputByte(IDE_REG_ALTSTATUS(uIoBase)), IoInputByte(IDE_REG_ERROR(uIoBase)));

		if(BootIdeWriteAtapiData(uIoBase, pAtapiCommandPacket12Bytes, 12)) {
			printk("  Drive %d:BootIdeIssueAtapiPacketCommandAndPacket 3 FAILED, error=%02X\n", nDriveIndex, IoInputByte(IDE_REG_ERROR(uIoBase)));
			return 1;
		}

		if(BootIdeWaitDataReady(uIoBase)) {
			printk("  Drive %d:  BootIdeIssueAtapiPacketCommandAndPacket Atapi Wait for data ready FAILED, status=0x%02X, error=0x%02X\n",
				nDriveIndex, IoInputByte(IDE_REG_ALTSTATUS(uIoBase)), IoInputByte(IDE_REG_ERROR(uIoBase)));
			return 1;
		}

		return 0;
}


int BootIdeReadData(unsigned uIoBase, void * buf, size_t size)
{
	WORD * ptr = (WORD *) buf;

	if (BootIdeWaitDataReady(uIoBase)) {
		printk_debug("data not ready...\n");
		return 1;
	}

	while (size > 1) {
		*ptr++ = IoInputWord(IDE_REG_DATA(uIoBase));
		size -= 2;
	}

	IoInputByte(IDE_REG_STATUS(uIoBase));

    if(IoInputByte(IDE_REG_ALTSTATUS(uIoBase)) & 0x01) return 2;

	return 0;
}

// Issues a block of data ATA-style but with some small delays


/////////////////////////////////////////////////
//  BootIdeDriveInit
//
//  Called by BootIdeInit for each drive
//  detects and inits each drive, and the structs containing info about them

static int BootIdeDriveInit(unsigned uIoBase, int nIndexDrive)
{
	tsIdeCommandParams tsicp = IDE_DEFAULT_COMMAND;
	unsigned short* drive_info;
	BYTE baBuffer[512];

	tsaHarddiskInfo[nIndexDrive].m_fwPortBase = uIoBase;
	tsaHarddiskInfo[nIndexDrive].m_wCountHeads = 0u;
	tsaHarddiskInfo[nIndexDrive].m_wCountCylinders = 0u;
	tsaHarddiskInfo[nIndexDrive].m_wCountSectorsPerTrack = 0u;
	tsaHarddiskInfo[nIndexDrive].m_dwCountSectorsTotal = 1ul;
	tsaHarddiskInfo[nIndexDrive].m_bLbaMode = IDE_DH_CHS;
	tsaHarddiskInfo[nIndexDrive].m_fDriveExists = 0;
	tsaHarddiskInfo[nIndexDrive].m_enumDriveType=EDT_UNKNOWN;
	tsaHarddiskInfo[nIndexDrive].m_fAtapi=false;

	tsicp.m_bDrivehead = IDE_DH_DEFAULT | IDE_DH_HEAD(0) | IDE_DH_CHS | IDE_DH_DRIVE(nIndexDrive);
	IoOutputByte(IDE_REG_DRIVEHEAD(uIoBase), tsicp.m_bDrivehead);

	if(BootIdeWaitNotBusy(uIoBase)) {
			printk_debug("  Drive %d: Not Ready\n", nIndexDrive);
			return 1;
	}

	if(!nIndexDrive) // this should be done by ATAPI sig detection, but I couldn't get it to work
	{ // master... you have to send it IDE_CMD_GET_INFO
		int nReturn=0;
		if(BootIdeIssueAtaCommand(uIoBase, IDE_CMD_GET_INFO, &tsicp)) {
			nReturn=IoInputByte(IDE_REG_CYLINDER_MSB(uIoBase))<<8;
			nReturn |=IoInputByte(IDE_REG_CYLINDER_LSB(uIoBase));
//			printk("nReturn = %x\n", nReturn);

			if(nReturn!=0xeb14) {
				printk("  Drive %d: detect FAILED, error=%02X\n", nIndexDrive, IoInputByte(IDE_REG_ERROR(uIoBase)));
				return 1;
			}
		}

	} else { // slave... death if you send it IDE_CMD_GET_INFO, it needs an ATAPI request

			if(BootIdeIssueAtaCommand(uIoBase, 0xa1, &tsicp)) {
				printk("  Drive %d: detect FAILED, error=%02X\n", nIndexDrive, IoInputByte(IDE_REG_ERROR(uIoBase)));
				return 1;
			}

	}

	BootIdeWaitDataReady(uIoBase);
	BootIdeReadData(uIoBase, baBuffer, IDE_SECTOR_SIZE);

	drive_info = (unsigned short*)baBuffer;
	tsaHarddiskInfo[nIndexDrive].m_wCountHeads = drive_info[3];
	tsaHarddiskInfo[nIndexDrive].m_wCountCylinders = drive_info[1];
	tsaHarddiskInfo[nIndexDrive].m_wCountSectorsPerTrack = drive_info[6];
	tsaHarddiskInfo[nIndexDrive].m_dwCountSectorsTotal = *((unsigned int*)&(drive_info[60]));

	{ int n;  // get rid of trailing spaces, add terminating '\0'
		WORD * pw=(WORD *)&(drive_info[10]);
		for(n=0; n<20;n+=2) { tsaHarddiskInfo[nIndexDrive].m_szSerial[n]=(*pw)>>8; tsaHarddiskInfo[nIndexDrive].m_szSerial[n+1]=(char)(*pw); pw++; }
		n=19; while(tsaHarddiskInfo[nIndexDrive].m_szSerial[n]==' ') n--; tsaHarddiskInfo[nIndexDrive].m_szSerial[n+1]='\0';
		pw=(WORD *)&(drive_info[27]);
		for(n=0; n<40;n+=2) { tsaHarddiskInfo[nIndexDrive].m_szIdentityModelNumber[n]=(*pw)>>8;tsaHarddiskInfo[nIndexDrive].m_szIdentityModelNumber[n+1]=(char)(*pw); pw++; }
		n=39; while(tsaHarddiskInfo[nIndexDrive].m_szIdentityModelNumber[n]==' ') n--; tsaHarddiskInfo[nIndexDrive].m_szIdentityModelNumber[n+1]='\0';
	}
	tsaHarddiskInfo[nIndexDrive].m_fDriveExists = 1;


	if(tsaHarddiskInfo[nIndexDrive].m_wCountHeads==0) { // CDROM/DVD


		tsaHarddiskInfo[nIndexDrive].m_fAtapi=true;

		printk("  %d: %s %s\n",
			nIndexDrive,
			tsaHarddiskInfo[nIndexDrive].m_szIdentityModelNumber,
			tsaHarddiskInfo[nIndexDrive].m_szSerial
		);


		{  // this is the only way to clear the ATAPI ''I have been reset'' error indication
			BYTE ba[128];
			int nPacketLength=BootIdeAtapiAdditionalSenseCode(nIndexDrive, &ba[0], sizeof(ba));
			if(nPacketLength<12) {
				printk("Unable to get ASC from drive\n");
				return 1;
			}
//			printk("ATAPI Drive reports ASC 0x%02X\n", ba[12]);  // normally 0x29 'reset' but clears the condition by reading
			nPacketLength=BootIdeAtapiAdditionalSenseCode(nIndexDrive, &ba[0], sizeof(ba));
			if(nPacketLength<12) {
				printk("Unable to get ASC from drive\n");
				return 1;
			}
//			printk("ATAPI Drive reports ASC 0x%02X\n", ba[12]);
			if(ba[12]==0x3a) { // no media, this is normal if there is no CD in the drive

			}
		}

	} else { // HDD

		unsigned long ulDriveCapacity1024=(tsaHarddiskInfo[nIndexDrive].m_wCountHeads * ((tsaHarddiskInfo[nIndexDrive].m_wCountCylinders * tsaHarddiskInfo[nIndexDrive].m_wCountSectorsPerTrack)/1024))/2;
		printk("  %d: %s %s (%u/%u/%u)=%u.%uGB, Sec=%04X\n",
			nIndexDrive,
			tsaHarddiskInfo[nIndexDrive].m_szIdentityModelNumber,
			tsaHarddiskInfo[nIndexDrive].m_szSerial,
 			tsaHarddiskInfo[nIndexDrive].m_wCountHeads,
   		tsaHarddiskInfo[nIndexDrive].m_wCountCylinders,
   		tsaHarddiskInfo[nIndexDrive].m_wCountSectorsPerTrack,
			ulDriveCapacity1024/1000, ulDriveCapacity1024%1000,
			drive_info[128]
		);
	
	}


	if(drive_info[128]) { // 'security' is in force, unlock the drive
		BYTE baMagic[0x200], baKeyFromEEPROM[0x10], baEeprom[0x30];
		int n;
		int nEepHash;
		tsIdeCommandParams tsicp1 = IDE_DEFAULT_COMMAND;
		DWORD dwMagicFromEEPROM;
		void genHDPass(
			BYTE * beepkey,
			unsigned char *HDSerial,
			unsigned char *HDModel,
			unsigned char *HDPass
		);
		DWORD BootHddKeyGenerateEepromKeyData(
			BYTE *eeprom_data,
			BYTE *HDKey
		);
		int nEepromAttempts=3;  // three goes at getting uncorrupted EEPROM only

		BootCpuCache(true);  // operate at good speed

		dwMagicFromEEPROM=0;
		while((dwMagicFromEEPROM==0) && (nEepromAttempts--)) {

			nEepHash=0;

			for(n=0;n<0x30;n++) {
				int nValue=-1;
				while(nValue<0) {
					nValue=I2CTransmitByteGetReturn(0x54, n);
					if(nValue<0) { int n1=20; while(n1--) Delay();  }  // in event of error, have a little sit down
				}
				baEeprom[n]=(BYTE)nValue;
				nEepHash+=baEeprom[n];
			}
//			DumpAddressAndData(0, &baEeprom[0], 0x30);
			dwMagicFromEEPROM = BootHddKeyGenerateEepromKeyData( &baEeprom[0], &baKeyFromEEPROM[0]);  // 0 = something screwed with EEPROM read
		}

		if(nEepromAttempts<1) {
			printk("Corrupted EEPROM!\n");
			DumpAddressAndData(0, &baEeprom[0], 0x30);
			while(1) ;
		}

			// clear down the unlock packet, except for b8 set in first word (high security unlock)

		for(n=0;n<0x200;n++) baMagic[n]=0;
		baMagic[1]=0x01;

		genHDPass( baKeyFromEEPROM, tsaHarddiskInfo[nIndexDrive].m_szSerial, tsaHarddiskInfo[nIndexDrive].m_szIdentityModelNumber, &baMagic[2]);

			if(BootIdeWaitNotBusy(uIoBase)) {
					printk_debug("  %d:  Not Ready\n", nIndexDrive);
					return 1;
			}
			tsicp1.m_bDrivehead = IDE_DH_DEFAULT | IDE_DH_HEAD(0) | IDE_DH_CHS | IDE_DH_DRIVE(nIndexDrive);

				if(BootIdeIssueAtaCommand(uIoBase, IDE_CMD_SECURITY_UNLOCK, &tsicp1)) {
					printk_debug("  %d:  when issuing unlock command FAILED, error=%02X\n", nIndexDrive, IoInputByte(IDE_REG_ERROR(uIoBase)));
					return 1;
				}
			BootIdeWaitDataReady(uIoBase);
			BootIdeWriteData(uIoBase, &baMagic[0], IDE_SECTOR_SIZE);
//			DumpAddressAndData(0, &baMagic[0], 0x30);

			if (BootIdeWaitNotBusy(uIoBase))	{ printk_debug("error on BootIdeIssueAtaCommand wait 1\n"); return 1; }
				// check that we are unlocked

				tsicp.m_bDrivehead = IDE_DH_DEFAULT | IDE_DH_HEAD(0) | IDE_DH_CHS | IDE_DH_DRIVE(nIndexDrive);
			if(BootIdeIssueAtaCommand(uIoBase, IDE_CMD_GET_INFO, &tsicp)) {
				printk_debug("  %d:  on issuing get status after unlock detect FAILED, error=%02X\n", nIndexDrive, IoInputByte(IDE_REG_ERROR(uIoBase)));
				return 1;
			}
					BootIdeWaitDataReady(uIoBase);
			if(BootIdeReadData(uIoBase, baBuffer, IDE_SECTOR_SIZE)) {
				printk_debug("  %d:  BootIdeReadData FAILED, error=%02X\n", nIndexDrive, IoInputByte(IDE_REG_ERROR(uIoBase)));
				return 1;
			}
			if(drive_info[128]&0x0004) {
				printk("  %d:  FAILED to unlock drive, security: %04x\n", nIndexDrive, drive_info[128]);
			} else {
//				printk("  %d:  Drive unlocked, new sec=%04X\n", nIndexDrive, drive_info[128]);
			}
	}

	if (drive_info[49] & 0x200) { /* bit 9 of capability word is lba supported bit */
		tsaHarddiskInfo[nIndexDrive].m_bLbaMode = IDE_DH_LBA;
	} else {
		tsaHarddiskInfo[nIndexDrive].m_bLbaMode = IDE_DH_CHS;
	}


	if(tsaHarddiskInfo[nIndexDrive].m_wCountHeads) { // HDD not DVD (that shows up as 0 heads)

		unsigned char ba[512];

			// report on the FATX-ness of the drive contents

		if(BootIdeReadSector(nIndexDrive, &ba[0], 3, 0, 512)) {
			printk("     Unable to get FATX sector\n");
		} else {
			if( (ba[0]=='B') && (ba[1]=='R') && (ba[2]=='F') && (ba[3]=='R') ) {
				tsaHarddiskInfo[nIndexDrive].m_enumDriveType=EDT_UNKNOWN;
				printk("      FATX Formatted Drive\n", nIndexDrive);
			} else {
				printk("      Non-native Drive\n", nIndexDrive);
			}
		}

			// report on the MBR-ness of the drive contents

		if(BootIdeReadSector(nIndexDrive, &ba[0], 0, 0, 512)) {
			printk("     Unable to get first sector\n");
		} else {
			if( (ba[0x1fe]==0x55) && (ba[0x1ff]==0xaa) ) {
				printk("      MBR Present\n", nIndexDrive);
			} else {
				printk("      No MBR at start of drive\n", nIndexDrive);
			}
		}
	}

	return 0;
}


/////////////////////////////////////////////////
//  BootIdeInit
//
//  Called at boot-time to init and detect connected devices

int BootIdeInit(void)
{

	printk("Initializing IDE Controller\n");

	BootIdeDriveInit(IDE_BASE1, 0);
	BootIdeDriveInit(IDE_BASE1, 1);

	return 0;
}


/////////////////////////////////////////////////
//  BootIdeAtapiAdditionalSenseCode
//
//  returns the ATAPI extra error info block

int BootIdeAtapiAdditionalSenseCode(int nDriveIndex, BYTE * pba, int nLengthMaxReturn) {
	unsigned 	uIoBase = tsaHarddiskInfo[nDriveIndex].m_fwPortBase;

		BYTE ba[2048];
		int nReturn;
		memset(&ba[0], 0, 12);
		ba[0]=0x03;
	 	ba[4]=0xff;

		if(BootIdeIssueAtapiPacketCommandAndPacket(nDriveIndex, &ba[0])) {
			BYTE bStatus=IoInputByte(IDE_REG_ALTSTATUS(uIoBase)), bError=IoInputByte(IDE_REG_ERROR(uIoBase));
			printk("  Drive %d: BootIdeAtapiAdditionalSenseCode 3 Atapi Wait for data ready FAILED, status=%02X, error=0x%02X, ASC unavailable\n", nDriveIndex, bStatus, bError);
			return 1;
		}

		nReturn=IoInputByte(IDE_REG_CYLINDER_LSB(uIoBase));
		nReturn |=IoInputByte(IDE_REG_CYLINDER_MSB(uIoBase))<<8;
		if(nReturn>nLengthMaxReturn) nReturn=nLengthMaxReturn;
		BootIdeReadData(uIoBase, pba, nReturn);

		return nReturn;
}

/////////////////////////////////////////////////
//  BootIdeReadSector
//
//  Read an absolute sector from the device
//  knows if it should use ATA or ATAPI according to HDD or DVD
//  This is the main function for reading things from a drive

int BootIdeReadSector(int nDriveIndex, void * pbBuffer, unsigned int block, int byte_offset, int n_bytes) {
	tsIdeCommandParams tsicp = IDE_DEFAULT_COMMAND;
	unsigned uIoBase;
	unsigned char baBufferSector[IDE_SECTOR_SIZE];
	unsigned int track;
	int status;

	uIoBase = tsaHarddiskInfo[nDriveIndex].m_fwPortBase;

	tsicp.m_bDrivehead = IDE_DH_DEFAULT | IDE_DH_HEAD(0) | IDE_DH_CHS | IDE_DH_DRIVE(nDriveIndex);
	IoOutputByte(IDE_REG_DRIVEHEAD(uIoBase), tsicp.m_bDrivehead);

	if ((nDriveIndex < 0) || (nDriveIndex >= 2) ||
	    (tsaHarddiskInfo[nDriveIndex].m_fDriveExists == 0))
	{
		printk_debug("unknown drive\n");
		return 1;
	}

	if(tsaHarddiskInfo[nDriveIndex].m_fAtapi) {

		BYTE ba[12];
		int nReturn;

		if(n_bytes<2048) {
			printk("Must have 2048 byte sector for ATAPI!!!!!\n");
			return 1;
		}

		tsicp.m_wCylinder=2048;
		memset(&ba[0], 0, 12);
		ba[0]=0x28; ba[2]=block>>24; ba[3]=block>>16; ba[4]=block>>8; ba[5]=block; ba[7]=0; ba[8]=1;

		if(BootIdeIssueAtapiPacketCommandAndPacket(nDriveIndex, &ba[0])) {
			bprintf("Unable to issue ATAPI command\n");
			return 1;
		}

		nReturn=IoInputByte(IDE_REG_CYLINDER_LSB(uIoBase));
		nReturn |=IoInputByte(IDE_REG_CYLINDER_MSB(uIoBase))<<8;
//		printk("nReturn = %x\n", nReturn);

		if(nReturn>2048) nReturn=2048;
		BootIdeReadData(uIoBase, pbBuffer, nReturn);

		return 0;
	}

	if (tsaHarddiskInfo[nDriveIndex].m_wCountHeads > 8) {
		IoOutputByte(IDE_REG_CONTROL(uIoBase), 0x0a);
	} else {
		IoOutputByte(IDE_REG_CONTROL(uIoBase), 0x02);
	}

	tsicp.m_bCountSector = 1;

	if (tsaHarddiskInfo[nDriveIndex].m_bLbaMode == IDE_DH_CHS) {
		track = block / tsaHarddiskInfo[nDriveIndex].m_wCountSectorsPerTrack;

		tsicp.m_bSector = 1+(block % tsaHarddiskInfo[nDriveIndex].m_wCountSectorsPerTrack);
		tsicp.m_wCylinder = track / tsaHarddiskInfo[nDriveIndex].m_wCountHeads;
		tsicp.m_bDrivehead = IDE_DH_DEFAULT |
			IDE_DH_HEAD(track % tsaHarddiskInfo[nDriveIndex].m_wCountHeads) |
			IDE_DH_DRIVE(nDriveIndex) |
			IDE_DH_CHS;
	} else {

		tsicp.m_bSector = block & 0xff; /* lower byte of block (lba) */
		tsicp.m_wCylinder = (block >> 8) & 0xffff; /* middle 2 bytes of block (lba) */
		tsicp.m_bDrivehead = IDE_DH_DEFAULT | /* set bits that must be on */
			((block >> 24) & 0x0f) | /* lower nibble of byte 3 of block */
			IDE_DH_DRIVE(nDriveIndex) |
			IDE_DH_LBA;
	}

	if(BootIdeIssueAtaCommand(uIoBase, IDE_CMD_READ_MULTI_RETRY, &tsicp)) {
		printk_debug("ide error %02X...\n", IoInputByte(IDE_REG_ERROR(uIoBase)));
		return 1;
	}
	if (n_bytes != IDE_SECTOR_SIZE) {
		status = BootIdeReadData(uIoBase, baBufferSector, IDE_SECTOR_SIZE);
		if (status == 0) {
			memcpy(pbBuffer, baBufferSector+byte_offset, n_bytes);
		}
	} else {
		status = BootIdeReadData(uIoBase, pbBuffer, IDE_SECTOR_SIZE);
	}
	return status;
}



///////////////////////////////////////////////
//      BootIdeBootSectorHddOrElTorito
//
//  Attempts to load boot code from Hdd or from CDROM/DVDROM
//   If HDD, loads MBR from Sector 0, if CDROM, uses El Torito to load default boot sector
//
// returns 0 if *pbaResult loaded with (512-byte/Hdd, 2048-byte/Cdrom) boot sector
//  otherwise nonzero return indicates error type

int BootIdeBootSectorHddOrElTorito(int nDriveIndex, BYTE * pbaResult)
{
	static const BYTE baCheck11hFormat[] = {
			0x00,0x43,0x44,0x30,0x30,0x31,0x01,0x45,
			0x4C,0x20,0x54,0x4F,0x52,0x49,0x54,0x4F,
			0x20,0x53,0x50,0x45,0x43,0x49,0x46,0x49,
			0x43,0x41,0x54,0x49,0x4F,0x4E
	};
	int n;
	DWORD * pdw;

	if(tsaHarddiskInfo[nDriveIndex].m_fAtapi) {

/******   Numbnut's guide to El Torito CD Booting   ********

  Sector 11h of a bootable CDROM looks like this (11h is a magic number)
  The DWORD starting at +47h is the sector index of the 'boot catalog'

00000000: 00 43 44 30 30 31 01 45 : 4C 20 54 4F 52 49 54 4F    .CD001.EL TORITO
00000010: 20 53 50 45 43 49 46 49 : 43 41 54 49 4F 4E 00 00     SPECIFICATION..
00000020: 00 00 00 00 00 00 00 00 : 00 00 00 00 00 00 00 00    ................
00000030: 00 00 00 00 00 00 00 00 : 00 00 00 00 00 00 00 00    ................
00000040: 00 00 00 00 00 00 00 13 : 00 00 00 00 00 00 00 00    ................
*/

		if(BootIdeReadSector(nDriveIndex, &pbaResult[0], 0x11, 0, 2048)) {
			bprintf("Unable to get first sector\n");
			return 1;
		}

		for(n=0;n<sizeof(baCheck11hFormat);n++) {
			if(pbaResult[n]!=baCheck11hFormat[n]) {
				bprintf("Sector 11h not bootable format\n");
				return 2;
			}
		}

		pdw=(DWORD *)&pbaResult[0x47];

/*
At sector 13h (in this example only), the boot catalog:

00000000: 01 00 00 00 4D 69 63 72 : 6F 73 6F 66 74 20 43 6F    ....Microsoft Co
00000010: 72 70 6F 72 61 74 69 6F : 6E 00 00 00 4C 49 55 AA    rporation...LIU.
(<--- validation entry)
00000020: 88 00 00 00 00 00 04 00 : 25 01 00 00 00 00 00 00    ........%.......
(<-- initial/default entry - 88=bootable, 04 00 = 4 x (512-byte virtual sectors),
  = 1 x 2048-byte CDROM sector in boot, 25 01 00 00 = starts at sector 0x125)
*/

		if(BootIdeReadSector(nDriveIndex, &pbaResult[0], *pdw, 0, 2048)) {
			bprintf("Unable to get boot catalog\n");
			return 3;
		}

		if((pbaResult[0]!=1) || (pbaResult[0x1e]!=0x55) || (pbaResult[0x1f]!=0xaa)) {
			bprintf("Boot catalog header corrupt\n");
			return 4;
		}

		if(pbaResult[0x20]!=0x88) {
			bprintf("Default boot catalog entry is not bootable\n");
			return 4;
		}

		pdw=(DWORD *)&pbaResult[0x28];
/*
And so at sector 0x125 (in this example only), we finally see the boot code

00000000: FA 33 C0 8E D0 BC 00 7C : FB 8C C8 8E D8 52 E8 00    .3.....|.....R..
00000010: 00 5E 81 EE 11 00 74 12 : 81 FE 00 7C 75 75 8C C8    .^....t....|uu..
00000020: 3D 00 00 75 7F EA 37 00 : C0 07 C6 06 AE 01 33 90    =..u..7.......3.
...
000007E0: 00 00 00 00 00 00 00 00 : 00 00 00 00 00 00 00 00    ................
000007F0: 00 00 00 00 00 00 00 00 : 00 00 00 00 00 00 55 AA    ..............U.
*/

		if(BootIdeReadSector(nDriveIndex, &pbaResult[0], *pdw, 0, 2048)) {
			bprintf("Unable to get boot catalog\n");
			return 3;
		}

		if((pbaResult[0x7fe]!=0x55) || (pbaResult[0x7ff]!=0xaa)) {
			bprintf("Boot sector does not have boot signature!\n");
			return 4;
		}

		return 0; // success

	} else { // HDD boot

		if(BootIdeReadSector(nDriveIndex, &pbaResult[0], 0, 0, 512)) {
			bprintf("Unable to get MBR\n");
			return 3;
		}

		if((pbaResult[0x1fe]!=0x55) || (pbaResult[0x1ff]!=0xaa)) {
			bprintf("Boot sector does not have boot signature!\n");
			return 4;
		}

		return 0; // succes
	}
}

