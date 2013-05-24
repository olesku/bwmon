/*
	bwmon - Simple bandwidth monitor for linux.
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>

#define CLS "\033[2J\033[1;1H"

typedef struct {
	char dev[32];
	long rxBytes;
	long txBytes;
	long rxPackets;
	long txPackets;
} bwinfo;

int fetchData(bwinfo **bwi, const char* iface) {
	bwinfo* p;
	FILE *fp = NULL;
	char buf[1024], *bufp, c, *tok, *tmp;
	int ifCount = 0, i;


	*bwi = (bwinfo*)malloc(sizeof(bwinfo));
	p = *bwi;

	fp = fopen("/proc/net/dev", "r");
	if (fp == NULL) {
		perror("Could not open /proc/net/dev");
		exit(1);
	}

	while (!feof(fp)) {

		/* fgets() seems to ignore the last line for some reason, so we'll roll our own instead. */
		for (bufp = buf; (fread(&c, 1, 1, fp)) > 0 && c != '\n' && bufp < buf+1024; *bufp++ = c);
		*bufp++ = '\0';

		if (buf[0]== '\0') break; /* No data. */
		if (!strchr(buf, ':')) continue; /* No interface name. */

		tok = strtok(buf, " ");
		if (tok == NULL)  {
			fprintf(stderr, "Error, no tokens in buffer.\n", buf);
			exit(1);
		}
		
		i = 1;	

		do {
			switch(i) {
				case 1: /* Interface name. */
					tmp = strchr(tok, ':');

					if (tmp == NULL) {
						fprintf(stderr, "Error fetching interface name, token: %s.\n", tok);
						exit(1);
					}

					if ( tmp-tok > 32 ) {
						fprintf(stderr, "Error fetcing interface name, buffer to small. Token: %s\n", tok);
						exit(1);
					}

					strncpy(p->dev, tok, tmp-tok);
					p->dev[tmp-tok] = '\0';

					/* Grr, seems like the format of /proc/net/dev is not consistent.. so we need this extra code. */
					if (tmp != tok+strlen(tok)-1) {
						tmp++;
						p->rxBytes = strtoul(tmp, NULL, 10);
						i = 3;
						continue;
					}
				break;

				case 2: /* Recvbytes. */
					p->rxBytes = strtoul(tok, NULL, 10);
				break;

				case 3: /* Recvpackets. */
					p->rxPackets = strtoul(tok, NULL, 10);
				break;

				case 10: /* Txbytes. */
					p->txBytes = strtoul(tok, NULL, 10);
				break;

				case 11: /* Txpackets. */
					p->txPackets = strtoul(tok, NULL, 10);
				break;
			}

			if (errno == ERANGE) {
				fprintf(stderr, "Error converting data with at token %i.\n", i);
				perror("strtol");
				exit(1);
           	}

			tok = strtok(NULL, " ");
			i++;
		} while(tok != NULL);
		
	
			ifCount++;
			*bwi = (bwinfo*)realloc(*bwi, sizeof(bwinfo) * (ifCount + 1));
			p = *(bwi)+ifCount;
	}

	/* Set up dummy values for the last element. */
	memset(p->dev, 0, 32);
	p->rxBytes = p->rxPackets = p->txBytes = p->txPackets = -1;

	fclose(fp);
	return ifCount;
}

int main(int argc, char* argv[]) {
	bwinfo *bwi_first = NULL, *bwi_second = NULL, *p1, *p2;
	int ifCount, i;

	while(1) {
		ifCount = fetchData(&bwi_first, NULL);
		usleep(1000 * 1000);
		fetchData(&bwi_second, NULL);
	
		printf(CLS);

		for (p1 = bwi_first, p2 = bwi_second; p1 < (bwi_first+ifCount); p1++, p2++) {
			printf("%s:\tRX: %.2f\tkB/s\tTX: %.2f\tkB/s\n", p1->dev, 
				   (double)(p2->rxBytes - p1->rxBytes) / 1024, 
				   (double)(p2->txBytes - p1->txBytes) / 1024,
				   p1->txBytes, p2->txBytes);
		}

		free(bwi_first);
		free(bwi_second);
	}

	return 0;
}
