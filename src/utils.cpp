#include "utils.h"
#include <sys/types.h>
#include <sys/stat.h>
#include <string.h>

/* XXX mode should be mode_t */

void
strmode ( int mode, char *p )
{
	/* print type */
	switch ( mode & S_IFMT )
	{
		case S_IFDIR:			/* directory */
			*p++ = 'd';
			break;
		case S_IFCHR:			/* character special */
			*p++ = 'c';
			break;
		case S_IFBLK:			/* block special */
			*p++ = 'b';
			break;
		case S_IFREG:			/* regular */
			*p++ = '-';
			break;
		case S_IFLNK:			/* symbolic link */
			*p++ = 'l';
			break;
//#ifdef S_IFSOCK
		case S_IFSOCK:			/* socket */
			*p++ = 's';
			break;
//#endif
//#ifdef S_IFIFO
		case S_IFIFO:			/* fifo */
			*p++ = 'p';
			break;
//#endif
		default:			/* unknown */
			*p++ = '?';
			break;
	}
	/* usr */
	if ( mode & S_IRUSR )
		*p++ = 'r';
	else
		*p++ = '-';
	if ( mode & S_IWUSR )
		*p++ = 'w';
	else
		*p++ = '-';
	switch ( mode & ( S_IXUSR | S_ISUID ) )
	{
		case 0:
			*p++ = '-';
			break;
		case S_IXUSR:
			*p++ = 'x';
			break;
		case S_ISUID:
			*p++ = 'S';
			break;
		case S_IXUSR | S_ISUID:
			*p++ = 's';
			break;
	}
	/* group */
	if ( mode & S_IRGRP )
		*p++ = 'r';
	else
		*p++ = '-';
	if ( mode & S_IWGRP )
		*p++ = 'w';
	else
		*p++ = '-';
	switch ( mode & ( S_IXGRP | S_ISGID ) )
	{
		case 0:
			*p++ = '-';
			break;
		case S_IXGRP:
			*p++ = 'x';
			break;
		case S_ISGID:
			*p++ = 'S';
			break;
		case S_IXGRP | S_ISGID:
			*p++ = 's';
			break;
	}
	/* other */
	if ( mode & S_IROTH )
		*p++ = 'r';
	else
		*p++ = '-';
	if ( mode & S_IWOTH )
		*p++ = 'w';
	else
		*p++ = '-';
	switch ( mode & ( S_IXOTH | S_ISVTX ) )
	{
		case 0:
			*p++ = '-';
			break;
		case S_IXOTH:
			*p++ = 'x';
			break;
		case S_ISVTX:
			*p++ = 'T';
			break;
		case S_IXOTH | S_ISVTX:
			*p++ = 't';
			break;
	}
	*p++ = ' ';		/* will be a '+' if ACL's implemented */
	*p = '\0';
}

int     is_dir(char *path)
{
    struct stat sb;

    /* XXX: report errors? */
    if (stat(path, &sb) == -1)
        return(0);

    return(S_ISDIR(sb.st_mode));
}

int is_reg(char *path)
{
    struct stat sb;

    if (stat(path, &sb) == -1)
    {
        return (0);
    }
    return(S_ISREG(sb.st_mode));
}

void  fxp_local_do_ls(const char *args ,std::vector<std::map<char, std::string> > & fileinfos  )
{
    int sz ;
    std::map<char,std::string> thefile;
    char file_size[32];
    char file_date[64];
    char file_type[32];
    char fname[PATH_MAX+1];
    char the_path[PATH_MAX+1];
    
    struct tm *ltime;
    struct stat thestat ;    
    
    DIR * dh = opendir(args);
    struct dirent * entry = NULL ;
    fileinfos.clear();
    
    while( ( entry = readdir(dh)) != NULL )
    {
        thefile.clear();
        memset(&thestat,0,sizeof(thestat));
        strcpy(the_path,args);
        strcat(the_path,"/");
        strcat(the_path,entry->d_name);
        if(strcmp(entry->d_name,".") == 0) goto out_point;
        if(strcmp(entry->d_name,"..") == 0) goto out_point ;

        if(stat(the_path,&thestat) != 0 ) continue;
        ltime = localtime(&thestat.st_mtime);
        
        sprintf(file_size,"%llu", thestat.st_size);
        strmode(thestat.st_mode,file_type);
        if (ltime != NULL) {
            if (time(NULL) - thestat.st_mtime < (365*24*60*60)/2)
                sz = strftime(file_date, sizeof file_date, "%Y/%m/%d %H:%M:%S", ltime);
            else
                sz = strftime(file_date, sizeof file_date, "%Y/%m/%d %H:%M:%S", ltime);
        } 
        strcpy(fname,entry->d_name);
        thefile.insert(std::make_pair('N',std::string(fname)));
        thefile.insert(std::make_pair('T',std::string(file_type)));
        thefile.insert(std::make_pair('S',std::string(file_size)));
        thefile.insert(std::make_pair('D',std::string( file_date )));
        
        fileinfos.push_back(thefile);
        
        out_point:

                continue ;
    }
    closedir(dh);
}

int     fxp_local_do_mkdir(const char * path )
{
    int ret = 0 ;
    
    ret = mkdir(path,0777);
    
    return ret ;
}