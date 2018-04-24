/*

    EnergyMech, IRC bot software
    Copyright (c) 1997-2018 proton

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.

*/
#define DNS_C
#include "config.h"

#ifdef RAWDNS

#include "defines.h"
#include "structs.h"
#include "global.h"
#include "h.h"
#include "text.h"
#include "mcmd.h"

#define unpack_uint16_t(x)	(((x)[0] << 8) | ((x)[1]))
#define unpack_uint32_t(x)	(((x)[0] << 24) | ((x)[1] << 16) | ((x)[2] << 8) | ((x)[3]))

typedef struct dnsType
{
	uint16_t	type;
	uint16_t	class;

} dnsType;

typedef struct dnsRType
{
	uint16_t	type;		/* &0 */
	uint16_t	class;		/* &2 */
	uint32_t	ttl;		/* &4 */
	uint16_t	rdlength;	/* &8 */

} dnsRType;

#define DNS_QUERY		1
#define DNS_TYPE_A		1
#define DNS_CLASS_IN		1
#define DNS_TYPE_NS		2
#define DNS_TYPE_CNAME		5

#define	QUERY_FLAGS		128

#define MAX_QUESTIONS		16

LS int dnssock = -1;
LS int dnsserver = 0;

#ifdef DEBUG
char *type_textlist[] =
{ NULL, "A", "NS", "MD", "MF", "CNAME", "SOA", "MB", "MG", "MR", "NULL", "WKS", "PTR", "HINFO", "MINFO", "MX", "TXT", };
#endif /* DEBUG */

void init_rawdns(void)
{
	struct	sockaddr_in sai;

	if ((dnssock = socket(AF_INET,SOCK_DGRAM,0)) < 0)
		return;

	memset(&sai,0,sizeof(sai));
	sai.sin_addr.s_addr = INADDR_ANY;
	sai.sin_family = AF_INET;

	if (bind(dnssock,(struct sockaddr*)&sai,sizeof(sai)) < 0)
	{
		close(dnssock);
		dnssock = -1;
		return;
	}
	SockFlags(dnssock);
#ifdef DEBUG
	debug("(init_rawdns) {%i} dnssock is active\n",dnssock);
#endif /* DEBUG */
}

struct in_addr dnsroot_lookup(const char *hostname)
{
	dnsAuthority *da;
	struct in_addr ip;

	for(da=dnsroot;da;da=da->next)
	{
		if (!stringcasecmp(hostname,da->hostname))
		{
#ifdef DEBUG
			debug("(dnsroot_lookup) %s = %s\n",hostname,inet_ntoa(da->ip));
#endif /* DEBUG */
			return(da->ip);
		}
	}
	ip.s_addr = -1;
	return(ip);
}

const char *get_dns_token(const char *src, const char *packet, char *dst, int sz)
{
	const char *endsrc = NULL;
	uint16_t offptr;
	int	tsz;
	int	dot = 0;

	for(;;dot=1)
	{
		tsz = (uchar)*(src++);
		if ((tsz & 0xC0) == 0xC0)
		{
			offptr = (tsz & 0x3f) << 8;
			offptr |= *src;
			if ((packet + offptr) > (packet + sz))
				return(src+1);
			if (!endsrc)
				endsrc = src + 1;
			src = packet + offptr;
			tsz = *(src++);
		}
		if (tsz == 0)
			break;
		if (dot)
			*(dst++) = '.';
		while(tsz)
		{
			tsz--;
			*(dst++) = *(src++);
		}
	}
	*dst = 0;
	return((endsrc) ? endsrc : src);
}

int make_query(char *packet, const char *hostname)
{
	dnsQuery *h;
	char	*size,*dst;

	/*
	 *  make a packet
	 */
	memset(packet,0,12);
	h = (dnsQuery*)packet;
	h->qid = rand() & 0xffff;
	h->flags = htons(0x0100);; /* Query = 0, Recursion Desired = 1 */
	h->questions = htons(1);
	size = packet + 12;
	dst  = size + 1;
	while(*hostname)
	{
		if ((*dst = *hostname) == '.')
		{
			*size = (dst - size - 1);
			size = dst;
		}
		hostname++;
		dst++;
	}
	*size = (dst - size - 1);

	dst[0] = 0;
	dst[1] = 0;
	dst[2] = 1;
	dst[3] = 0;
	dst[4] = 1;
	return(dst - packet + 5);
}

#ifndef UNALIGNED_MEM

struct in_addr temp_ip_data;

struct in_addr *get_stored_ip(const char *ipdata)
{
	memcpy((char *)&temp_ip_data,ipdata,4);
	return(&temp_ip_data);
}

#else

#define get_stored_ip(x)	((struct in_addr *)x)

#endif /* UNALIGNED_MEM */

/*
 *
 */
#ifdef SCRIPTING

void dns_hook(char *host, char *resolved)
{
	Hook	*hook;
	Mech	*backbot;

	backbot = current;
	for(hook=hooklist;hook;hook=hook->next)
	{
		if (hook->flags == MEV_DNSRESULT && !stringcasecmp(host,hook->type.host))
		{
			for(current=botlist;current;current=current->next)
			{
				if (hook->guid == 0 || hook->guid == current->guid)
				{
					hook->func(hook->type.host,resolved,hook);
				}
			}
		}
	}
	current = backbot;
}

#endif /* SCRIPTING */

void parse_query(int psz, dnsQuery *query)
{
	struct	sockaddr_in sai;
	char	packet[512];
	Mech	*backupbot;
	struct in_addr *ip;
	dnsList	*dns;
	const char *src,*rtyp;
	char	token[64],token2[64];
	int	sz,n;

	src = (const char*)query;

#ifdef DEBUG
	for(n=0;n<16;n++)
		token[n] = (ntohs(query->flags) & (1<<n)) ? '1' : '0';
	token[16] = 0;
	n = ntohs(query->flags);
	debug("(parse_query) %i: flags = %i { %s %i %s%s%s%s%s }\n",
		sz,n,token,
		(n&15), /* result code */
		(n&32768)  ? "QR 1 (Answer) ":"QR 0 (Question) ",
		(n&1024) ? "AA ":"",
		(n&512) ? "TC ":"",
		(n&256)? "RD ":"",
		(n&128)? "RA ":"");
#endif /* DEBUG */

	src += 12;

	for(dns=dnslist;dns;dns=dns->next)
	{
		if (dns->id == ntohs(query->qid))
			break;
	}
	if (!dns)
		return;


	n = ntohs(query->questions);
	while(n--)
	{
		/* skip QNAME */
		src = get_dns_token(src,(const char *)query,token,psz);
		/* skip (uint16_t)QTYPE and (uint16_t)QCLASS */
		src += 4;
	}

	n = ntohs(query->answers);
	while(n)
	{
		src = get_dns_token(src,(const char*)query,token,psz);
		rtyp = src;
		src += 10;

#ifdef DEBUG
		debug("(parse_query) %i: answer = %s\n",dns->id,token);
#endif /* DEBUG */

		if ((unpack_uint16_t(&rtyp[0]) == DNS_TYPE_CNAME) &&
		    (unpack_uint16_t(&rtyp[2]) == DNS_CLASS_IN))
		{
			get_dns_token(src,(const char *)query,token2,psz);
#ifdef DEBUG
			debug("(parse_query) %i: cname: %s = %s\n",dns->id,token,token2);
#endif /* DEBUG */
			if (dns->cname)
				Free((char**)&dns->cname);
			dns->when = now + 30;
			set_mallocdoer(parse_query);
			dns->cname = stringdup(token2);
		}

		if ((unpack_uint16_t(&rtyp[0]) == DNS_TYPE_A) &&
		    (unpack_uint16_t(&rtyp[2]) == DNS_CLASS_IN) &&
		    (unpack_uint16_t(&rtyp[8]) == 4))
		{
			ip = get_stored_ip(src);
			if (dns->auth && !stringcasecmp(dns->auth->hostname,token))
			{
				dns->auth->ip.s_addr = ip->s_addr;
				dns->when = now + 60;
#ifdef DEBUG
				debug("(parse_query) a auth: %s = %s\n",token,inet_ntoa(*ip));
#endif /* DEBUG */
			}
			else
			if (!stringcasecmp(dns->host,token) || (dns->cname && !stringcasecmp(dns->cname,token)))
			{
				dns->ip.s_addr = ip->s_addr;
				dns->when = now + 3600;
#ifdef DEBUG
				debug("(parse_query) a: %s = %s\n",token,inet_ntoa(*ip));
#endif /* DEBUG */
				/* a final dns anwer was received */
				backupbot = current;
				for(current=botlist;current;current=current->next)
				{
					send_pa(PA_DNS|PA_END,token,"Resolved: %s (%s)",token,inet_ntoa(*ip));
				}
#ifdef SCRIPTING
				dns_hook(token,inet_ntoa(*ip));
#endif /* SCRIPTING */
				current = backupbot;
				return;
			}
		}
		src += unpack_uint16_t(&rtyp[8]);
		n--;
	}

	n = ntohs(query->authorities);
	sz = (n > 1) ? RANDOM(1,n) : 1;
#ifdef DEBUG
	if (n)
		debug("(parse_query) auth: select %i count %i\n",sz,n);
#endif /* DEBUG */
	while(n)
	{
		src = get_dns_token(src,(const char*)query,token,psz);
		rtyp = src;
		src += 10;
		if ((unpack_uint16_t(&rtyp[0]) == DNS_TYPE_NS) &&
		    (unpack_uint16_t(&rtyp[2]) == DNS_CLASS_IN))
		{
			dnsAuthority *da;

			get_dns_token(src,(const char *)query,token2,psz);
			if (sz == n)
			{
				if (dns->auth == NULL)
				{
					set_mallocdoer(parse_query);
					da = dns->auth = (dnsAuthority*)Calloc(sizeof(dnsAuthority) + strlen(token2));
					/* Calloc sets to zero da->ip.s_addr = 0; */
					stringcpy(da->hostname,token2);
				}
				else
				if (dns->findauth == 1)
				{
					dns->findauth = 2;
					if (dns->auth2)
						Free((char**)&dns->auth2);
					set_mallocdoer(parse_query);
					da = dns->auth2 = (dnsAuthority*)Calloc(sizeof(dnsAuthority) + strlen(token2));
					/* Calloc sets to zero da->ip.s_addr = 0; */
					stringcpy(da->hostname,token2);
#ifdef DEBUG
					debug("(parse_query) 2nd auth set: %s\n",token2);
#endif /* DEBUG */
				}
			}
#ifdef DEBUG
			debug("(parse_query) authorities: %s = %s%s\n",token,token2,(sz==n) ? MATCH_ALL : "");
#endif /* DEBUG */
		}
#ifdef DEBUG
		else
		{
			debug("(parse_query) DNS TYPE %s(%i), CLASS %i, size %i\n",
				type_textlist[unpack_uint16_t(&rtyp[0])],unpack_uint16_t(&rtyp[0]),
				unpack_uint16_t(&rtyp[2]),unpack_uint16_t(&rtyp[8]));
		}
#endif /* DEBUG */
		src += unpack_uint16_t(&rtyp[8]);
		n--;
	}

	if (dns->findauth >= 1)
		dns->findauth = 1;

	n = ntohs(query->resources);
	while(n)
	{
		src = get_dns_token(src,(const char*)query,token,psz);
		rtyp = src;
		src += 10;

		if (	(unpack_uint16_t(&rtyp[0]) == DNS_TYPE_A) &&
			(unpack_uint16_t(&rtyp[2]) == DNS_CLASS_IN) &&
			(unpack_uint16_t(&rtyp[8]) == 4))
		{
			ip = get_stored_ip(src);
			if (dns->auth && !stringcasecmp(dns->auth->hostname,token))
				dns->auth->ip.s_addr = ip->s_addr;
			if (dns->auth2 && !stringcasecmp(dns->auth2->hostname,token))
				dns->auth2->ip.s_addr = ip->s_addr;
#ifdef DEBUG
			debug("(parse_query) resources: %s = %s\n",token,inet_ntoa(*ip));
#endif /* DEBUG */
		}
		src += unpack_uint16_t(&rtyp[8]);
		n--;
	}

	if (dns->auth && dns->auth->ip.s_addr == 0)
	{
		sai.sin_addr = dnsroot_lookup(dns->auth->hostname);
		if (sai.sin_addr.s_addr != -1)
			dns->auth->ip.s_addr = sai.sin_addr.s_addr;
	}

	if (dns->auth2 && dns->auth2->ip.s_addr == 0)
	{
		sai.sin_addr = dnsroot_lookup(dns->auth2->hostname);
		if (sai.sin_addr.s_addr != -1)
			dns->auth2->ip.s_addr = sai.sin_addr.s_addr;
	}

	if (dns->auth && dns->auth->ip.s_addr && dns->auth2)
	{
		Free((char**)&dns->auth2);
		dns->findauth = 0;
	}

#ifdef DEBUG
	debug("> dns->when	%lu\n",dns->when);
	debug("> dns->ip	%s\n",inet_ntoa(dns->ip));
	debug("> dns->auth	%s : %s (%i)\n",(dns->auth) ? dns->auth->hostname : NULLSTR,
		(dns->auth) ? inet_ntoa(dns->auth->ip) : "-",(dns->auth) ? dns->auth->count : 0);
	debug("> dns->auth2	%s : %s (%i)\n",(dns->auth2) ? dns->auth2->hostname : NULLSTR,
		(dns->auth2) ? inet_ntoa(dns->auth2->ip) : "-",(dns->auth2) ? dns->auth2->count : 0);
	debug("> dns->findauth	%i\n",dns->findauth);
	debug("> dns->id	%i\n",dns->id);
	debug("> dns->cname	%s\n",nullstr(dns->cname));
	debug("> dns->host	%s\n",dns->host);
#endif /* DEBUG */

	src = NULL;
	if (dns->auth2)
	{
		if (dns->auth2->ip.s_addr && dns->auth)
		{
			src = dns->auth->hostname;
			sai.sin_addr.s_addr = dns->auth2->ip.s_addr;
		}
		else
		{
			src = dns->auth2->hostname;
			sai.sin_addr.s_addr = (ia_ns[dnsserver].s_addr == 0) ? ia_default.s_addr : ia_ns[dnsserver].s_addr;
			if (++dns->auth->count >= MAX_QUESTIONS)
			{
#ifdef DEBUG
				debug("(parse_query) dns->auth->count >= 32, starting over\n");
#endif /* DEBUG */
				/* too many questions about who the authorative dns server is, start from scratch */
				Free((char**)&dns->auth);
				Free((char**)&dns->auth2);
				dns->findauth = 0;
				src = (dns->cname) ? dns->cname : dns->host;
			}
		}
	}
	else
	if (dns->auth)
	{
		if (dns->auth->ip.s_addr)
		{
			/*
			 *  we know the IP of the authorative NS to ask
			 */
			src = (dns->cname) ? dns->cname : dns->host;
			sai.sin_addr.s_addr = dns->auth->ip.s_addr;
		}
		else
		{
			/*
			 *  have to dig up the IP of the NS to ask
			 */
			dns->findauth = 1;
			src = dns->auth->hostname;
			sai.sin_addr.s_addr = (ia_ns[dnsserver].s_addr == 0) ? ia_default.s_addr : ia_ns[dnsserver].s_addr;
			if (++dns->auth->count >= MAX_QUESTIONS)
			{
#ifdef DEBUG
				debug("(parse_query) dns->auth->count >= 32, starting over\n");
#endif /* DEBUG */
				/* too many questions about who the authorative dns server is, start from scratch */
				Free((char**)&dns->auth);
				Free((char**)&dns->auth2);
				dns->findauth = 0;
				src = (dns->cname) ? dns->cname : dns->host;
			}
		}
	}
	if (src)
	{
		dns->id = rand();
#ifdef DEBUG
		debug("(parse_query) %i: asking %s who is `%s'\n",dns->id,inet_ntoa(sai.sin_addr),src);
#endif /* DEBUG */
		sz = make_query(packet,src);
		dns->when = now + 60;
		sai.sin_family = AF_INET;
		sai.sin_port = htons(53);
		((dnsQuery*)packet)->qid = htons(dns->id);
		if (sendto(dnssock,packet,sz,0,(struct sockaddr*)&sai,sizeof(sai)) < 0)
		{
			close(dnssock);
			dnssock = -1;
		}
	}
	if (dns->auth && dns->auth->ip.s_addr)
		Free((char**)&dns->auth);
	if (dns->auth2 && dns->auth2->ip.s_addr)
		Free((char**)&dns->auth2);
	if (src == NULL && dns->ip.s_addr == 0 && dns->cname && dns->host && dns->auth == NULL && dns->auth2 == NULL)
	{
		dns->id = rand();
		sai.sin_addr.s_addr = (ia_ns[dnsserver].s_addr == 0) ? ia_default.s_addr : ia_ns[dnsserver].s_addr;
#ifdef DEBUG
		debug("(parse_query) %i: asking %s who is `%s' (CNAME question)\n",dns->id,inet_ntoa(sai.sin_addr),dns->cname);
#endif /* DEBUG */
		sz = make_query(packet,dns->cname);
		dns->when = now + 60;
		sai.sin_family = AF_INET;
		sai.sin_port = htons(53);
		((dnsQuery*)packet)->qid = htons(dns->id);
		if (sendto(dnssock,packet,sz,0,(struct sockaddr*)&sai,sizeof(sai)) < 0)
		{
			close(dnssock);
			dnssock = -1;
		}
	}
}

void rawdns(const char *hostname)
{
	struct	sockaddr_in sai;
	dnsQuery *query;
	dnsList *item;
	char	packet[512];
	int	sz;

	if (dnssock == -1)
		init_rawdns();

	if (dnssock == -1)
		return;

	sz = make_query(packet,hostname);
	query = (dnsQuery*)packet;

	set_mallocdoer(rawdns);
	item = (dnsList*)Calloc(sizeof(dnsList) + strlen(hostname));
	stringcpy(item->host,hostname);
	item->id = ntohs(query->qid);
	item->when = now + 30;
	item->next = dnslist;
	dnslist = item;

	/*
	 *  send the packet
	 */
	sai.sin_family = AF_INET;
	sai.sin_port = htons(53);
	sai.sin_addr.s_addr = (ia_ns[dnsserver].s_addr == 0) ? ia_default.s_addr : ia_ns[dnsserver].s_addr;

#ifdef DEBUG
	debug("(rawdns) questions %s: %s\n",inet_ntoa(sai.sin_addr),item->host);
#endif /* DEBUG */

	dnsserver++;
	if (ia_ns[dnsserver].s_addr == 0)
		dnsserver = 0;

	if (sendto(dnssock,packet,sz,0,(struct sockaddr*)&sai,sizeof(sai)) < 0)
	{
		close(dnssock);
		dnssock = -1;
	}
}

void select_rawdns(void)
{
	dnsList *dns,**pdns;

	if (dnssock != -1)
	{
		chkhigh(dnssock);
		FD_SET(dnssock,&read_fds);
	}
restart:
	pdns = &dnslist;
	while(*pdns)
	{
		if ((*pdns)->when < now)
		{
			dns = *pdns;
			if (dns->cname)
				Free((char**)&dns->cname);
			if (dns->auth)
				Free((char**)&dns->auth);
			if (dns->auth2)
				Free((char**)&dns->auth2);
#ifdef DEBUG
			debug("(select_rawdns) removing %s qid %i\n",dns->host,dns->id);
#endif /* DEBUG */
			for(current=botlist;current;current=current->next)
			{
				send_pa(PA_DNS|PA_END,dns->host,"Unable to resolve %s",dns->host);
			}
#ifdef SCRIPTING
			dns_hook(dns->host,"~");
#endif /* SCRIPTING */
			*pdns = dns->next;
			Free((char**)&dns);
			goto restart;
		}
		pdns = &(*pdns)->next;
	}
}

void process_rawdns(void)
{
	struct	sockaddr_in sai;
	char	packet[512];
	int	sz,n;

	if (dnssock == -1)
		return;
	if (FD_ISSET(dnssock,&read_fds))
	{
		sz = sizeof(sai);
		n = recvfrom(dnssock,packet,512,0,(struct sockaddr*)&sai,&sz);
		if (n < sizeof(dnsQuery))
			return;
#ifdef DEBUG
		debug("(process_rawdns) packet from: %s (%i bytes)\n",inet_ntoa(sai.sin_addr),n);
#endif /* DEBUG */
		parse_query(n,(dnsQuery*)packet);
	}
}

char *poll_rawdns(char *hostname)
{
	dnsList *dns;

	for(dns=dnslist;dns;dns=dns->next)
	{
		if (dns->ip.s_addr && !stringcasecmp(dns->host,hostname))
		{
#ifdef DEBUG
			debug("(poll_rawdns) a: %s ==> %s\n",hostname,inet_ntoa(dns->ip));
#endif /* DEBUG */
			return(inet_ntoa(dns->ip));
		}
	}
	return(NULL);
}

LS int backup_debug;

int read_dnsroot(char *line)
{
	struct	in_addr ia;
	dnsAuthority *da;
	char	*name,*a,*ip,*src;

	name = chop(&line);
	a    = chop(&line);	/* TTL is optional */
	if (a && stringcmp(a,"A"))
		a = chop(&line);
	ip   = chop(&line);

	if (a && !stringcmp(a,"A") && ip && inet_aton(ip,&ia) != 0)
	{
		/* remove trailing dot */
		for(src=name;*src;)
		{
			if (*src == '.' && *(src+1) == 0)
				*src = 0;
			else
				src++;
		}
		set_mallocdoer(read_dnsroot);
		da = (dnsAuthority*)Calloc(sizeof(dnsAuthority) + strlen(name));
		stringcpy(da->hostname,name);
		da->ip.s_addr = ia.s_addr;
		da->next = dnsroot;
		dnsroot = da;
#ifdef DEBUG
		dodebug = backup_debug;
		debug("(read_dnsroot) stored root IP: %s = %s\n",name,ip);
		dodebug = 0;
#endif /* DEBUG */
	}
	return(FALSE);
}

/*
 *  find the IP quickly
 */
uint32_t rawdns_get_ip(const char *host)
{
	uint32_t ip;

	if ((ip = inet_addr(host)) == INADDR_NONE)
	{
	}
#ifdef DEBUG
	debug("(rawdns_get_ip) %s -> %s\n",host,inet_ntoa(*((struct in_addr*)&ip)));
#endif /* DEBUG */
	return(ip);
}

/*
 *
 *  commands related to DNS
 *
 */

void do_dnsroot(COMMAND_ARGS)
{
	int	in;

	if ((in = open(rest,O_RDONLY)) >= 0)
	{
#ifdef SESSION
		Strp *p;

		p = (Strp*)Calloc(strlen(rest)+1);
		p->next = dnsrootfiles;
		stringcpy(p->p,rest);
		dnsrootfiles = p;
#endif /* SESSION */
#ifdef DEBUG
		backup_debug = dodebug;
		dodebug = 0;
#endif /* DEBUG*/
		readline(in,&read_dnsroot);			/* readline closes in */
#ifdef DEBUG
		dodebug = backup_debug;
#endif /* DEBUG */
	}
}

void do_dnsserver(COMMAND_ARGS)
{
	/*
	 *  on_msg checks: GAXS
	 */
	struct in_addr ia;
	char	*p,c,tempservers[MAX_NAMESERVERS*16+3];
		/* (xxx.yyy.zzz.www + 1 space * MAX_NAMESERVERS) + 1 terminator char + 2 chars bold font */
	int	i;

	if (!*rest)
	{
		*(p = tempservers) = 0;
		for(i=0;i<MAX_NAMESERVERS;i++)
		{
			if (ia_ns[i].s_addr > 0)
			{
				if (i == dnsserver)
				{
					sprintf(p,"\037%s\037 ",inet_ntoa(ia_ns[i]));
					p = STREND(p);
				}
				else
				{
					p = stringcpy(p,inet_ntoa(ia_ns[i]));
					*(p++) = ' ';
					*p = 0;
				}
			}
		}
		if (*tempservers == 0)
			stringcpy(tempservers,"\037127.0.0.1\037");
		to_user(from,"Current DNS Servers: %s",tempservers);
		return;
	}

	c = *(rest++);
	if ((ia.s_addr = inet_addr(rest)) == INADDR_NONE)
		c = 0;

	switch(c)
	{
	case '+':
		for(i=0;i<MAX_NAMESERVERS;i++)
		{
			if (ia_ns[i].s_addr == 0)
			{
				ia_ns[i].s_addr = ia.s_addr;
				to_user(from,"DNS Server added: %s",rest);
				return;
			}
		}
		to_user(from,"No free DNS Server slots found, remove one before adding new servers");
		return;
	case '-':
		for(i=0;i<MAX_NAMESERVERS;i++)
		{
			if (ia_ns[i].s_addr == ia.s_addr || ia.s_addr == 0)
			{
				ia_ns[i].s_addr = 0;
			}
		}
		for(i=1;i<MAX_NAMESERVERS;i++)
		{
			if (ia_ns[i-1].s_addr == 0)
			{
				ia_ns[i-1].s_addr = ia_ns[i].s_addr;
				ia_ns[i].s_addr = 0;
			}
		}
		dnsserver = 0;
		if (ia.s_addr > 0)
			to_user(from,"DNS Server removed: %s",rest);
		else
			to_user(from,"All known DNS Servers removed.");
		return;
	default:
		usage(from);
	}
}

/*
 *  resolve the dns of a user/host
 *  usage: DNS <nick|host>
 */
void do_dns(COMMAND_ARGS)
{
	/*
	 *  on_msg checks: CARGS and GAXS
	 */
	char	*host,*res,*src,*dst,*dot;
	uint32_t ip;

	/* to date, all hostnames contain atleast one dot */
	if ((STRCHR(rest,'.')))
	{
		host = rest;
	}
	else
	{
		/* no dots, try to find it as a nick */
		/* searches all channels and nicks, clobbers get_nuh  */
		if ((host = find_nuh(rest)) == NULL)
		{
			to_user(from,"Unable to resolve %s: unknown nick/host",rest);
			return;
		}
		while(*host && *host != '@')
			host++;
		if (*host == '@')
			host++;
#ifdef DEBUG
		debug("(do_dns) %s is on host %s\n",rest,host);
#endif /* DEBUG */
	}
	if ((ip = inet_addr(host)) != INADDR_NONE)
	{
		/* flip an IP backwards to resolve hostname */
		/* a11.b22.c33.d44 */
		/* d44.c33.b22.a11.in-addr.arpa */
		dst = globaldata;
flipstep:
		src = host;
		dot = NULL;

		while(*src)
		{
			if (*src == '.')
				dot = src;
			src++;
		}
		if (dot)
		{
			*dot++ = 0; /* safe to modify buffer? */
			while(*dot)
				*dst++ = *dot++;
			*dst++ = '.';
			goto flipstep;
		}
		stringcpy(stringcpy(dst,host),".in-addr.arpa");
#ifdef DEBUG
		debug("(do_dns) host flipped to %s\n",globaldata);
#endif /* DEBUG */
		host = globaldata;
	}
	/* check if its in cache now */
	if ((res = poll_rawdns(host)))
	{
		/* Resolved: irc.dal.net (194.68.45.50) */
		send_pa(PA_DNS|PA_END,NULL,"Resolved: %s (%s)",host,res);
		return;
	}

	if (to && *to == '#')
		make_ireq(PA_DNS,to,host);
	else
		make_ireq(PA_DNS,from,host);
	rawdns(host);
}

#endif /* RAWDNS */
