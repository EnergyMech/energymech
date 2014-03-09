/*

    mkindex, create a binary index file of line offsets and lengths
    Copyright (c) 2001 proton

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
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/mman.h>
#include <fcntl.h>

int main(int argc, char **argv)
{
	char	ixname[1000];
	char	*map;
	char	*p,*beg,*end;
	off_t	sz;
	int	fd,ifd;
	int	n;

	if (argc < 2)
	{
		exit(1);
	}

	if ((fd = open(argv[1],O_RDONLY)) < 0)
	{
		fprintf(stderr,"open: %s:",argv[1]);
		perror("");
		exit(1);
	}

	strcpy(ixname,argv[1]);
	if ((p = strchr(ixname,'.')) == NULL)
		p = strchr(ixname,0);

	strcpy(p,".index");

	if (!strcmp(ixname,argv[1]))
	{
		fprintf(stderr,"input file is output file!\n");
		exit(1);
	}

	if ((ifd = open(ixname,O_WRONLY|O_TRUNC|O_CREAT,0600)) < 0)
	{
		fprintf(stderr,"open: %s:",ixname);
		perror("");
		exit(1);
	}

	sz = lseek(fd,0,SEEK_END);

	map = mmap(NULL,sz,PROT_READ,MAP_SHARED,fd,0);
	if (!map)
	{
		perror("mmap: ");
		exit(1);
	}

	end = map + sz;
	beg = p = map;

	n = 0;
	while(p < end)
	{
		if ((*p == '\n') || (*p == '\r'))
		{
			if (p != beg)
			{
				struct
				{
					int	off;
					int	sz;

				} entry;

				entry.off = beg - map;
				entry.sz = p - beg;

				if (entry.sz < 450)
				{
					write(ifd,&entry,sizeof(entry));
					n++;
				}
			}
			beg = p + 1;
		}
		p++;
	}

	close(fd);
	close(ifd);

	printf("%i entries indexed\n",n);
	exit(0);
}
