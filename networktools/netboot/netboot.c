#include "boot.h"
#include "lwip/debug.h"

#include "lwip/stats.h"

#include "lwip/tcp.h"
#include "memory_layout.h"
#include <shared.h>

static const char *requestGET  = "GET /sourceforge/xbox-linux/resctoox.t00x HTTP/1.0\n\n";
static const char *requestHEAD = "HEAD /sourceforge/xbox-linux/resctoox.t00x HTTP/1.0\n\n";
static int i = 0, initrdSize = 0, kernelSize = 0, hLen = 0, contLen = 0, fraction = 0;
static int head = 1, progCheck = 0, eoh = 0, fileLen = 0;
static char *tempBuf = (u8*)INITRD_START;
static char c[4], *appendLine = NULL, *header = NULL;
static struct ip_addr ipaddr;
static struct tcp_pcb *pcb = NULL;
static struct pbuf *q;
static u16_t port = 80;

static err_t handlePacklet(void *arg, struct tcp_pcb *pcb, err_t err);
static err_t recvPacklet(void *arg, struct tcp_pcb *pcb, struct pbuf *p, err_t err);
void processPayload(void);
static void connErr(void *arg, err_t err);
static int isdigit(char c);
void bootPacklet(void);
void processHeader(void);

static int isdigit(char c) {
	return ((c) >= '0' && (c) <= '9');
}

void processHeader() {
	char *headerPtr = strstr(header, "Content-Length");
	if(headerPtr != NULL) {
		headerPtr+=16;
		contLen = simple_strtol(headerPtr, NULL, 10);
//		printk("Content-Length: %i", contLen);
		free(header);
		fraction = (contLen / 64);
	} else {
		// Couldn't find the Content-Length.
		fraction = 250000;
	}
}

static void connErr(void *arg, err_t err) {
	printk("\n           Connection error");
	dots();
	cromwellError();
	printk("\n");
	while(1);
}

void processPayload() {
	int payloadIndex = fileLen;

	// Get to the kernel filesize.
	while(!isdigit(tempBuf[payloadIndex])) {
		payloadIndex--;
	}

	while(isdigit(tempBuf[payloadIndex])) {
		payloadIndex--;
	}
	payloadIndex++;
	kernelSize = simple_strtol(tempBuf+payloadIndex, NULL, 10);
	//printk("Kernel Size: %i\n", kernelSize);

	// Get to the initrd filesize.
	payloadIndex--;
	while(!isdigit(tempBuf[payloadIndex])) {
		payloadIndex--;
	}

	while(isdigit(tempBuf[payloadIndex])) {
		payloadIndex--;
	}
	payloadIndex++;
	initrdSize = simple_strtol(tempBuf+payloadIndex, NULL, 10);
	//printk("InitRD Size: %i\n", initrdSize);

	payloadIndex = initrdSize+kernelSize;

	// Get the "Append" information.
	i = 0;
	appendLine = (char*)malloc(1024);
	while(tempBuf[payloadIndex] != '|') {
		appendLine[i] = tempBuf[payloadIndex];
		i++;
		appendLine[i] = '\0';
		payloadIndex++;
	}
}

// Boot the Packlet
void bootPacklet() {
	eth_disable();
	memPlaceKernel(tempBuf+initrdSize, kernelSize);
	ExittoLinuxPacklet(initrdSize, appendLine);
}

static err_t recvPacklet(void *arg, struct tcp_pcb *pcb, struct pbuf *p, err_t err) {
	tcp_recved(pcb, p->tot_len);
	if(p == NULL) {
		tcp_close(pcb);
		if(head == 0) {
			printk("EOF\n");
			processPayload();
			bootPacklet();
		} else {
			head = 0;
//			printk("Header:\n%s\n", header);
			processHeader();
			pcb = tcp_new();
			tcp_err(pcb, connErr);
			tcp_setprio(pcb, TCP_PRIO_MAX);
			tcp_connect(pcb, &ipaddr, port, handlePacklet);
		}

	}
	if(eoh == 0) {
		c[0] = '\0';
		c[1] = '\0';
		c[2] = '\0';
		c[3] = '\0';
	}
	
	for (q = p; q; q = q->next) {
		for (i = 0; i < q->len; i++) {
			if(eoh == 0) {
				c[0] = c[1];
				c[1] = c[2];
				c[2] = c[3];
			}

			c[3] = ((char *)q->payload)[i];

			if(head == 1) {
				header[hLen] = c[3];
				hLen++;
				header[hLen] = '\0';
			}

			if(eoh == 1) {
				if(head == 0) {
					tempBuf[fileLen] = c[3];
					fileLen++;
					tempBuf[fileLen] = '\0';
					if((fileLen > progCheck) || (fileLen >= contLen)) {
						DisplayProgressBar(fileLen,contLen,0xffff0000);
						progCheck += fraction;
					}
				}
			}
			
			// Found the header.
			if(eoh == 0) {
				if((c[0] == '\r') && (c[1] == '\n') && (c[2] == '\r') && (c[3] == '\n')) {
					if(head == 0) {
						eoh = 1;
					}
					dots();
				}
			}
		}
	}
	pbuf_free(p);
	return ERR_OK;
}	

static err_t handlePacklet(void *arg, struct tcp_pcb *pcb, err_t err) {
	tcp_recv(pcb, recvPacklet);
	if(head == 1) {
		printk("           Contacting server");
		tcp_write(pcb, requestHEAD, strlen(requestHEAD), 0);
	} else {
		cromwellSuccess();
		printk("           Downloading Packlet");
		tcp_write(pcb, requestGET, strlen(requestGET), 0);
	}		
	return ERR_OK;
}

void netboot_init(int A, int B, int C, int D, int P) {
	contLen = progCheck = hLen = fileLen = eoh = 0;
	head = 1;
	contLen = 11*1024*1024;
	fraction = contLen/16;

	if(header != NULL) {
		free(header);
		header = NULL;
	}
	header = (char*)malloc(5120);

	memset(tempBuf, 0, 15*1024*1024);

	// HEANET
	//IP4_ADDR(&ipaddr, 193,1,193,66);
	// Custom
	IP4_ADDR(&ipaddr, A,B,C,D);
	port = (u16_t)P;

	if(pcb != NULL) {
		tcp_abort(pcb);
	}
	pcb = tcp_new();
	tcp_err(pcb, connErr);
	tcp_setprio(pcb, TCP_PRIO_MAX);
	tcp_connect(pcb, &ipaddr, port, handlePacklet);
	cromwellSuccess();
	printk("           Server: %i.%i.%i.%i:%i\n", A, B, C, D, P);
	downloadingLED();
}
