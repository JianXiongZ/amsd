//
// Created by root on 17-2-23.
//

#include <iostream>
#include <cstdio>
#include <cstdlib>
#include <cstring>

#include <unistd.h>
#include <fcntl.h>

#include <sys/types.h>
#include <sys/time.h>
#include <sys/signal.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <sys/un.h>
#include <arpa/inet.h>
#include <netinet/in.h>

#include <jansson.h>

using namespace std;

char *socket_path = (char *)"/tmp/.amsd_socket";

json_t *j_req = json_object();
json_t *j_req_data = json_object();

void help(){
	fprintf(stderr, "amsd_cli v0.1 - Command line interface for AMSD\n"
		"\n"
		"Usage: amsd_cli [-s <socket_path>] <operation> <sub-operation> [args...]\n"
		"\n"
		"Options:\n"
		"    -s        Specifies socket path instead of /tmp/.amsd_socket\n"
		"\n"
		"Operations:\n"
		"    controller:\n"
		"        list                            List controllers\n"
		"        add <ip:port> [<ip:port>...]    Add controller(s)\n"
		"        del <ip:port> [<ip:port>...]    Delete controller(s)\n"
		"    mailreport:\n"
		"        meow                            You must meow to send a mail report\n"
		"        anything                        Or if you really don't want to be cute...\n"
		"\n"
		"Examples:\n"
		"    # amsd_cli controller list\n"
		"    # amsd_cli -s /tmp/niconico_socket fwver 192.168.1.233\n");
}

void f_controller(char **argv){

	json_t *j_op = json_string(argv[0]);
	json_object_set_new(j_req_data, "op", j_op);

	if ((0 == strcmp(argv[0], "list"))||((0 == strcmp(argv[0], "wipe")))) {

	} else if ((0 == strcmp(argv[0], "add")) || (0 == strcmp(argv[0], "del"))) {
		size_t j = 1;
		char *delim;

		json_t *j_ctrls = json_array();
		json_t *j_ctrl;

		while (argv[j]) {
			delim = strchr(argv[j], ':');

			if (!delim) {
				cerr << "controller: error: bad ip:addr combination: " << argv[j] << endl;
				exit(2);
			}

			*delim = 0;

			j_ctrl = json_object();
			json_object_set_new(j_ctrl, "ip", json_string(argv[j]));
			json_object_set_new(j_ctrl, "port", json_integer(strtol(delim + 1, NULL, 10)));
			json_array_append_new(j_ctrls, j_ctrl);

			argv++;
		}

		json_object_set_new(j_req_data, "controllers", j_ctrls);
	} else {
		cerr << "controller: error: bad operation: " << argv[0] << endl;
		exit(2);
	}

}

string req(char *req){

	string ret;
	struct sockaddr_un remote_addr;
	int sfd = socket(AF_UNIX, SOCK_STREAM, 0);

	if (sfd < 1) {
		cerr << "error: socket() failed\n";
		exit(3);
	}

	memset(&remote_addr, 0, sizeof(struct sockaddr_un));

	remote_addr.sun_family = AF_UNIX;
	strncpy(remote_addr.sun_path, socket_path, sizeof(remote_addr.sun_path)-1);

	int cr = connect(sfd, (sockaddr *)&remote_addr, sizeof(struct sockaddr_un));

	if (cr != 0) {
		cerr << "error: connect() failed. Please check whether AMSD is running and the socket file exists.\n";
		exit(3);
	}

	send(sfd, req, strlen(req), MSG_WAITALL);
	send(sfd, "\n", 1, MSG_WAITALL);

	ssize_t n = 1;
	char rcvbuf[512] = {0};

	while (n > 0) {
		n = recv(sfd, rcvbuf, 511, 0);

		if (n < 1)
			break;

		rcvbuf[n] = 0;
		ret += rcvbuf;

	}

	close(sfd);
	return ret;

}

int main(int argc, char **argv){
	char **ops_argv = &argv[1];
	char *serialized_j_req;
	string ret;

	if (argv[1])
		if (0 == strcmp(argv[1], "lol")) {
			cerr << "THIS IS THE DOC.\n";
			exit(0);
		}

	if (argc < 3)
		goto badarg;

	if (0 == strcmp(argv[1], "-s")) {
		if (argc < 4)
			goto badarg;
		ops_argv = &argv[3];
	}

	json_object_set_new(j_req, "operation", json_string(ops_argv[0]));

	if (0 == strcmp(ops_argv[0], "controller")) {
		f_controller(ops_argv+1);
	} else if (0 == strcmp(ops_argv[0], "mailreport")) {

	} else {
		cerr << "error: " << ops_argv[0] << " is not an AMSD operation.\n";
		exit(2);
	}

	json_object_set_new(j_req, "data", j_req_data);

	serialized_j_req = json_dumps(j_req, 0);

	cerr << serialized_j_req << endl;

	ret = req(serialized_j_req);

	cerr << ret << endl;

	exit(0);

	badarg:
	help();
	exit(1);

}