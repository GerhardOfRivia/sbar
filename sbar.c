#define  _DEFAULT_SOURCE

#include <arpa/inet.h>
#include <ctype.h>
#include <ifaddrs.h>
#include <netdb.h>
#include <linux/if_link.h>

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <strings.h>

#include <sys/time.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/wait.h>

#include <time.h>
#include <unistd.h>
#include <X11/Xlib.h>

char *utcusa = "America/Denver";

static Display *dpy;

char *smprintf(char *fmt, ...) {
	va_list fmtargs;
	char *ret;
	int len;

	va_start(fmtargs, fmt);
	len = vsnprintf(NULL, 0, fmt, fmtargs);
	va_end(fmtargs);

	ret = malloc(++len * sizeof(char));
	if (ret == NULL) {
		perror("malloc");
		exit(1);
	}

	va_start(fmtargs, fmt);
	vsnprintf(ret, len, fmt, fmtargs);
	va_end(fmtargs);

	return ret;
}

char get_ac_status(){
	char *ac_status = "/sys/class/power_supply/AC/online";
	char acVal;

	FILE *ac_power_fp = NULL;
	if ((ac_power_fp = fopen(ac_status, "r")) == NULL) {
		perror("fopen error: ac_power_fp");
	}
	acVal = fgetc(ac_power_fp);
	fclose(ac_power_fp);
	return acVal;
}

float get_num_status(){

	FILE *ba_power_fp = NULL;;
	char *ba_status = "/sys/class/power_supply/BAT0/energy_now";
	char current_power[10];
	char** num_end = NULL;

	if ((ba_power_fp = fopen(ba_status, "r")) == NULL) {
		perror("fopen error: ba_power_fp");
	}

	fgets(current_power, 10, ba_power_fp);
	fclose(ba_power_fp);

	return (float)strtol(current_power, num_end, 10);
}

float get_dem_status(){

	FILE *po_power_fp = NULL;;
	char *po_status = "/sys/class/power_supply/BAT0/energy_full";
	char total_power[10];
	char** dem_end = NULL;

	if ((po_power_fp = fopen(po_status, "r")) == NULL) {
		perror("fopen error: po_power_fp");
	}

	fgets(total_power, 10, po_power_fp);
	fclose(po_power_fp);

	return (float)strtol(total_power, dem_end, 10);
}

char *get_status() {
	char acVal = get_ac_status();
	float num = get_num_status();
	float dem = get_dem_status();

	if (acVal == '1'){
		return smprintf("+%0.f%c", ((num*100)/dem), 37);
	}else{
		return smprintf("-%0.f%c", ((num*100)/dem), 37);
	}

}

void settz(char *tzname){
	setenv("TZ", tzname, 1);
}

char *mktimes(char *fmt, char *tzname){
	char buf[129];
	time_t tim;
	struct tm *timtm;

	memset(buf, 0, sizeof(buf));
	settz(tzname);
	tim = time(NULL);
	timtm = localtime(&tim);
	if (timtm == NULL) {
		perror("mktimes: localtime returned null");
	}

	if (!strftime(buf, sizeof(buf)-1, fmt, timtm)) {
		perror("strftime == 0");
	}

	return smprintf("%s", buf);
}

char* getaddr(){

	int family, s, n;
	struct ifaddrs *ifaddr, *ifa;
	if (getifaddrs(&ifaddr) == -1){
		fprintf(stderr, "getaddr: getifaddrs returned -1");
	}

	char host[NI_MAXHOST];
	char *ret_host = NULL;

	for(ifa = ifaddr, n = 0; ifa != NULL; ifa = ifa->ifa_next, n++) {
		if ((ifa->ifa_addr == NULL) || (strcmp(ifa->ifa_name, "lo") == 0)) continue;

		family = ifa->ifa_addr->sa_family;

		if (family == AF_INET) {
			s = getnameinfo(ifa->ifa_addr, (family == AF_INET) ? sizeof(struct sockaddr_in):sizeof(struct sockaddr_in6), host, NI_MAXHOST, NULL, 0, NI_NUMERICHOST);
			if (s != 0) {
				freeifaddrs(ifaddr);
				fprintf(stderr, "getnameinfo() failed.\n");
				return NULL;
			}
			if (ret_host == NULL) {
				ret_host = smprintf("%s: %s", ifa->ifa_name, host);
			} else {
				char *temp_host = ret_host;
				ret_host = smprintf("%s , %s: %s", ret_host, ifa->ifa_name, host);
				free(temp_host);
			}
		}
	}
	freeifaddrs(ifaddr);
    return ret_host;
}

char *loadavg(void){
	double avgs[3];

	if (getloadavg(avgs, 3) < 0) {
		perror("getloadavg");
		exit(1);
	}

	return smprintf("%.2f %.2f %.2f", avgs[0], avgs[1], avgs[2]);
}

void setStatus(char *str){
	XStoreName(dpy, DefaultRootWindow(dpy), str);
	XSync(dpy, False);
}

char* getStatus(int mode) {
	char* avgs;
	char* tmusa;
	char* pow;
	char* host;
	char* tempStatus;

	avgs = loadavg();
	host = getaddr();
	tmusa = mktimes("%a %d %b %H:%M %Z %Y", utcusa);

	if (mode & 1) {
		pow = get_status();
		tempStatus = smprintf("Load:%s | %s | %s | %s", avgs, host, pow, tmusa);
		free(pow);
	} else {
		tempStatus = smprintf("Load:%s | %s | %s", avgs, host, tmusa);
	}

	free(avgs);
	free(tmusa);
	free(host);
	
	return tempStatus;
}

int main(int argc, char* argv[]){

	int mode;
    int opt;
	char *status;

	if (!(dpy = XOpenDisplay(NULL))) {
		fprintf(stderr, "sbar: cannot open display.\n");
		return 1;
	}

	while ((opt = getopt(argc, argv, "b:")) != -1) {
		if (opt == 'b') {
			mode += 1;
		} else {
			fprintf(stderr, "Usage: %s [-b]\n", argv[0]);
			exit(EXIT_FAILURE);
		}
	}
	for (;;sleep(90)) {
		status = getStatus(mode);
		setStatus(status);
		free(status);
	}

	XCloseDisplay(dpy);

	return 0;
}

