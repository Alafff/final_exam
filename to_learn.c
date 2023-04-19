#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <netdb.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <stdlib.h>
#include <stdio.h>

int srvsd, maxsd, clidx[42 * 2048] = {0}, idx;
struct sockaddr_in srv;
fd_set csds, rsds, wsds;
char *msgs[42 * 2048] = {0}, wbuf[42 * 2048] = {0}, rbuf[42 * 2048] = {0};

void	failed() {
	char *err = "Fatal error\n";
	write(2, err, strlen(err));
	exit(EXIT_FAILURE);
}

void	sender(int from, char *msg)
{
	for (int sd = 0; sd <= maxsd; ++sd)
		if (FD_ISSET(sd, &wsds) && sd != from)
			send(sd, msg, strlen(msg), 0);
}
void	poster(int sd) {
	char *msg =	NULL;
	while (extract_message(&msgs[sd], &msg)) {
		sprintf(wbuf, "client %d: ", clidx[sd]);
		sender(sd, wbuf);
		sender(sd, msg);
		msg = NULL;
	}
}
int		adder() {
	int	sd = 0;
	socklen_t len = sizeof(srv);
	if ((sd = accept(srvsd, (struct sockaddr *)&srv, &len)) < 0)
		return(0);
	if (maxsd < sd)
		maxsd = sd;
	clidx[sd] = idx++;
	msgs[sd] = NULL;
	FD_SET(sd, &csds);
	sprintf(wbuf, "server: client %d just arrived\n", clidx[sd]);
	sender(sd, wbuf);
	return (1);
}

int		remover(int sd) {
	int ret = recv(sd, rbuf, 42 * 2048, 0);
	if (ret > 0) {
		rbuf[ret] = '\0';
		msgs[sd] = str_join(msgs[sd], rbuf);
		poster(sd);
		return (0);
	}
	sprintf(wbuf, "server: client %d just left\n", sd);
	sender(sd, wbuf);
	if (msgs[sd])
		free(msgs[sd]);
	msgs[sd] = NULL;
	FD_CLR(sd, &csds);
	close(sd);
	return (1);
}

void	serving() {
	while(1) {
		wsds = rsds = csds;
		if (select(maxsd + 1, &rsds, &wsds, NULL, NULL) < 0)
			failed();
		for (int sd = 0; sd <= maxsd; ++sd) {
			if (FD_ISSET(sd, &rsds)) {
				if (sd == srvsd && adder())
					break;
				else if(remover(sd))
					break;
			}
		}
	}
}

int		main(int ac, char **av) {
	if (ac != 2)
		write(2, "Wrong number of argument" ,strlen("Wrong number of argument"));
	else
	{
	bzero(&srv, sizeof(srv));
	srv.sin_family = AF_INET;
	srv.sin_addr.s_addr = htonl(2130706433); //127.0.0.1
	srv.sin_port = htons(atoi(av[1]));
	if ((srvsd = socket(AF_INET, SOCK_STREAM, 0)) < 0
	|| bind(srvsd, (const struct sockaddr *)&srv, sizeof(srv))
	|| listen(srvsd, SOMAXCONN))
		failed();
	maxsd = srvsd;
	FD_ZERO(&csds);
	FD_SET(srvsd, &csds);
	serving();
	}
}