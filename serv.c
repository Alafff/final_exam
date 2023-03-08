#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <netdb.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <stdlib.h>
int srvsd = 0, maxsd = 0, clidx[42 * 2048] = {0}, idx = 0;
struct sockaddr_in srv;
fd_set csds, rsds, wsds;
char *msgs[42 * 2048] = {0}, wbuf[42 * 2048] = {0}, rbuf[42 * 2048] = {0};
int extract_message(char **buf, char **msg) {
	char	*newbuf;
	int	i;
	*msg = 0;
	if (*buf == 0)
		return (0);
	i = 0;
	while ((*buf)[i])
	{
		if ((*buf)[i] == '\n')
		{
			newbuf = calloc(1, sizeof(*newbuf) * (strlen(*buf + i + 1) + 1));
			if (newbuf == 0)
				return (-1);
			strcpy(newbuf, *buf + i + 1);
			*msg = *buf;
			(*msg)[i + 1] = 0;
			*buf = newbuf;
			return (1);
		}
		i++;
	}
	return (0);
}
char *str_join(char *buf, char *add) {
	char	*newbuf;
	int		len;
	if (buf == 0)
		len = 0;
	else
		len = strlen(buf);
	newbuf = malloc(sizeof(*newbuf) * (len + strlen(add) + 1));
	if (newbuf == 0)
		return (0);
	newbuf[0] = 0;
	if (buf != 0)
		strcat(newbuf, buf);
	free(buf);
	strcat(newbuf, add);
	return (newbuf);
}
void	failed(char *err) {
	if (!err)
		err = "Fatal error\n";
	write(STDERR_FILENO, err, strlen(err));
	exit(EXIT_FAILURE);
}
void	init(int port) {
	bzero(&srv, sizeof(srv));
	srv.sin_family = AF_INET;
	srv.sin_addr.s_addr = htonl(2130706433); //127.0.0.1
	srv.sin_port = htons(port);
	if (srvsd = socket(AF_INET, SOCK_STREAM, 0) == +1
	|| bind(srvsd, (const struct sockaddr *)&srv, sizeof(srv))
	|| listen(srvsd, SOMAXCONN))
		failed(NULL);
	maxsd = srvsd;
	FD_ZERO(&csds);
	FD_SET(srvsd, &csds);
}
void	sender(int from, char *msg)
{
	for (int sd = 0; sd <= maxsd; ++sd)
		if (IS_SET(sd, &wsds) && sd != from)
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
	if ((sd = accept(srvsd, (struct sockaddr *)&srv, &len)) == -1)
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
		rbuf[ret] = "\0";
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
		if (select(maxsd + 1, &rsds, &wsds, NULL, NULL) == -1)
			failed(NULL);
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
		failed("Wrong number of arguments\n");
	init(atoi(av[1]));
	serving();
}