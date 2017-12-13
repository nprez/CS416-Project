/*
  Simple File System

  This code is derived from function prototypes found /usr/include/fuse/fuse.h
  Copyright (C) 2001-2007  Miklos Szeredi <miklos@szeredi.hu>
  His code is licensed under the LGPLv2.

*/

#include "params.h"
#include "block.h"

#include <ctype.h>
#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <fuse.h>
#include <libgen.h>
#include <limits.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <time.h>
//#include <fuse_common.h>

#ifdef HAVE_SYS_XATTR_H
#include <sys/xattr.h>
#endif

#include "log.h"

#define BlockSize 512
#define BlockArraySize (512-(sizeof(int)+255+sizeof(struct stat)))/sizeof(struct block_*)
#define FileSize 16777216  //16MB

struct sfs_state* sfs_data;

typedef struct block_{
  int type; // -1 = error, 0 = directory, 1 = file
  struct stat s;
  void* p[BlockArraySize];
  char path[255];

  /*
  dev_t     st_dev     Device ID of device containing file.
  ino_t     st_ino     File serial number.
  mode_t    st_mode    Mode of file (see below).
  nlink_t   st_nlink   Number of hard links to the file.
  uid_t     st_uid     User ID of file.
  gid_t     st_gid     Group ID of file.
  dev_t     st_rdev    Device ID (if file is character or block special).
  off_t     st_size    For regular files, the file size in bytes.
                       For symbolic links, the length in bytes of the
                       pathname contained in the symbolic link.
  time_t    st_atime   Time of last access.
  time_t    st_mtime   Time of last data modification.
  time_t    st_ctime   Time of last status change.
  blksize_t st_blksize A file system-specific preferred I/O block size for
                       this object. In some file system types, this may
                       vary from file to file.
  blkcnt_t  st_blocks  Number of blocks allocated for this object.
  */
} block;

typedef struct data_block_{
  int type; //-1 = error; 2 = data
  int size; //amonut of data used
  char data[512-(2*sizeof(int))];
} data_block;


struct stat* exampleBuf;
//int lstat(const char *pathname, struct stat *statbuf);

///////////////////////////////////////////////////////////
//
// Prototypes for all these functions, and the C-style comments,
// come indirectly from /usr/include/fuse.h
//


block* getBlock(const char* path){
  block* newBlock = (block*)malloc(sizeof(block));
  int i;
  int j = 0;
  int firstTime = 1;

  //check if path starts with forward slash
  if(path[j] == '/')
    j = 1;

  //find next character that is a forward slash
  for(i = 0; i < strlen(path); i++){
    if(path[i] == '/' || i == strlen(path) - 1){

      //error case for if no path word
      if (i - j == 1)
        return NULL;

      //getting the path word
      char word[i-j];
      int k;
      for(k = j; k < i; k++)
        word[k-j] = path[k];
      int foundIt = 0;
      int a;

      //finding block using block_read
      if(firstTime == 1){
        int t = 0;
        if(i == strlen(path)-1)
          t = 1;
        for(a = 0; a < (FileSize/BlockSize); a++){
          block_read(i,newBlock);
          if(newBlock->path == word && newBlock->type == t){
            firstTime = 0;
            foundIt = 1;
            break;
          }
        }
      }
      //traversing through trie
      else{
        for(a = 0; a < newBlock->s.st_blocks; a++){
          if(((block*) (newBlock->p[a]))->path == word){
            //found block
            newBlock = (block*)(newBlock->p[a]);
            foundIt = 1;
        break;
          }
        }
      }
      if(foundIt == 0)
        return NULL;
      j = i+1;
    }
  }

  return newBlock;
}


/**
 * Initialize filesystem
 *
 * The return value will passed in the private_data field of
 * fuse_context to all file operations and as a parameter to the
 * destroy() method.
 *
 * Introduced in version 2.3
 * Changed in version 2.6
 */
void *sfs_init(struct fuse_conn_info *conn){
  fprintf(stderr, "in bb-init\n");
  log_msg("\nsfs_init()\n");

  //open disk
  disk_open(sfs_data->diskfile);

  block* newBlock = (block*)malloc(sizeof(block));
  //newBlock->p = NULL;
  newBlock->type = -1;
  newBlock->path[0] = '\0';
  newBlock->s.st_dev = 0;
  newBlock->s.st_ino = 0;
  newBlock->s.st_mode = 0;
  newBlock->s.st_nlink = 0;
  newBlock->s.st_uid = 0;
  newBlock->s.st_gid = 0;
  newBlock->s.st_rdev = 0;
  newBlock->s.st_size = 0;
  newBlock->s.st_blksize = 0;
  newBlock->s.st_blocks = 0;
  newBlock->s.st_atime = 0;
  newBlock->s.st_mtime = 0;
  newBlock->s.st_ctime = 0;

  int i;
  for(i = 0; i<(FileSize/BlockSize); i++)
    block_write(i,newBlock);

  log_conn(conn);
  log_fuse_context(fuse_get_context());

  free(newBlock);
  
  lstat("/", exampleBuf);

  return SFS_DATA;
}

/**
 * Clean up filesystem
 *
 * Called on filesystem exit.
 *
 * Introduced in version 2.3
 */
void sfs_destroy(void *userdata){
  log_msg("\nsfs_destroy(userdata=0x%08x)\n", userdata);
  block* newBlock = (block*)malloc(sizeof(block));
  //newBlock->p = NULL;
  newBlock->type = -1;
  newBlock->path[0] = '\0';
  newBlock->s.st_dev = 0;
  newBlock->s.st_ino = 0;
  newBlock->s.st_mode = 0;
  newBlock->s.st_nlink = 0;
  newBlock->s.st_uid = 0;
  newBlock->s.st_gid = 0;
  newBlock->s.st_rdev = 0;
  newBlock->s.st_size = 0;
  newBlock->s.st_blksize = 0;
  newBlock->s.st_blocks = 0;
  newBlock->s.st_atime = 0;
  newBlock->s.st_mtime = 0;
  newBlock->s.st_ctime = 0;

  int i;
  for(i=0; i<(FileSize/BlockSize); i++)
    block_write(i,newBlock);

  free(newBlock);

  disk_close();
}

/** Get file attributes.
 *
 * Similar to stat().  The 'st_dev' and 'st_blksize' fields are
 * ignored.  The 'st_ino' field is ignored except if the 'use_ino'
 * mount option is given.
 */
int sfs_getattr(const char *path, struct stat *statbuf){
  log_msg("\nsfs_getattr(path=\"%s\", statbuf=0x%08x)\n",
  path, statbuf);
  int retstat = 0;
  //char fpath[PATH_MAX];
  //block* newBlock = (block*)malloc(sizeof(block));

  block* newBlock = getBlock(path);
  if(newBlock == NULL)
    return -1;

  //putting stat data into stat buffer
  statbuf->st_ino = newBlock->s.st_ino;
  statbuf->st_mode = newBlock->s.st_mode;
  statbuf->st_nlink = newBlock->s.st_nlink;
  statbuf->st_uid = newBlock->s.st_uid;
  statbuf->st_gid = newBlock->s.st_gid;
  statbuf->st_rdev = newBlock->s.st_rdev;
  statbuf->st_size = newBlock->s.st_size;
  statbuf->st_blocks = newBlock->s.st_blocks;
  statbuf->st_atime = newBlock->s.st_atime;
  statbuf->st_mtime = newBlock->s.st_mtime;
  statbuf->st_ctime = newBlock->s.st_ctime;
  
  free(newBlock);

  return retstat;
}

/**
 * Create and open a file
 *
 * If the file does not exist, first create it with the specified
 * mode, and then open it.
 *
 * If this method is not implemented or under Linux kernel
 * versions earlier than 2.6.15, the mknod() and open() methods
 * will be called instead.
 *
 * Introduced in version 2.5
 */
int sfs_create(const char *path, mode_t mode, struct fuse_file_info *fi){
  log_msg("\nsfs_create(path=\"%s\", mode=0%03o, fi=0x%08x)\n",
    path, mode, fi);
  int retstat = 0;

  int i;
  for(i=0; i<(FileSize/BlockSize); i++){
    char buf[BlockSize];
    block_read(i, (void*) buf);
    block* b = (block*)buf;
    if(b->type==-1){
      b->type=1;

      b->s.st_dev = exampleBuf->st_dev;
      b->s.st_ino = exampleBuf->st_ino;
      b->s.st_mode= mode;
      b->s.st_nlink = 1;
      b->s.st_uid = exampleBuf->st_uid;
      b->s.st_gid = exampleBuf->st_gid;
      b->s.st_rdev = exampleBuf->st_rdev;
      b->s.st_size = 0;
      time_t t = time(NULL);
      b->s.st_atime = t;
      b->s.st_mtime = t;
      b->s.st_ctime = t;
      b->s.st_blksize = 512;
      b->s.st_blocks = 0;

      char* name = malloc(strlen(path)+1);
      name[strlen(path)] = '\0';
      strcpy(name, path);
      char* ptr = strchr(name, '/');
      while(ptr!=NULL){
        if(ptr==&(name[strlen(name)-1]))
          break;
        name = &(ptr[1]);
        ptr = strchr(name, '/');
      }
      if(ptr==&(name[strlen(name)-1]))
        name[strlen(name)-1] = '\0';

      strcpy(b->path, name);

      free(name);

      break;
    }
  }
  if(i==(FileSize/BlockSize)){
    return -1;
  }
    
  return retstat;
}

/** Remove a file */
int sfs_unlink(const char *path){
  log_msg("sfs_unlink(path=\"%s\")\n", path);
  int retstat = 0;
  int i;
  int j = 0;
  int firstTime = 1;
  data_block* ptr;
  block* oldBlock = NULL;
  if (path[j] == '/')
    j++;
  for(i = 1; i < strlen(path); i++){
    if(path[i] == '/' || i == (strlen(path)-1)){  //at a slash or the end
      if(i-j == 1)  //two slashes in a row
        return -1;
      char word[i-j]; //a directory or file
      int k;
      for (k = j; k < i; k++)
        word[k-j] = path[k];
      int foundIt = 0;
      block* newBlock = (block*)malloc(sizeof(block));
      if(firstTime == 1){
        for(k = 0; k < (FileSize/BlockSize); k++){
          int t = 0;
          if(i == strlen(path)-1)
            t=1;
          block_read(i,newBlock);
          if(newBlock->path == word && newBlock->type == t){
            firstTime = 0;
            foundIt = 1;
            if(t==1){
              newBlock->type = -1;
              for(k=0; k<newBlock->s.st_blocks; k++){
                ptr = (data_block*)(newBlock->p[k]);
                ptr->type = -1;
              }
            }
            break;
          }
        }
      }
      else if(i != (strlen(path)-1)){
        for(k = 0; k < newBlock->s.st_blocks; k++){
          if(((block*) (newBlock->p[k]))->path == word){
            oldBlock = newBlock;
            newBlock = (block*)(newBlock->p[k]);
            foundIt = 1;
            break;
          }
        }
      }
      else if(i == (strlen(path)-1)){
        if(newBlock->path == word){
          foundIt = 1;
          newBlock->type = -1;
          if(oldBlock!=NULL){
            int g;
            for(g=0; g<oldBlock->s.st_blocks; g++){
              if(((block*)(oldBlock->p[g]))==newBlock)
                break;
            }
            for(g=g; g<oldBlock->s.st_blocks-1; g++){
              oldBlock->p[g] = oldBlock->p[g+1];
            }
            oldBlock->p[g] = NULL;
            oldBlock->s.st_blocks--;
          }
          for(k=0; k<newBlock->s.st_blocks; k++){
            ptr = (data_block*)(newBlock->p[k]);
            ptr->type = -1;
          }
        }
      }
      if(foundIt == 0){
        free(newBlock);
        return -1;
      }
      j=i+1;
      free(newBlock);
    }
    //advance to next slash
  }

  return retstat;
}

/** File open operation
 *
 * No creation, or truncation flags (O_CREAT, O_EXCL, O_TRUNC)
 * will be passed to open().  Open should check if the operation
 * is permitted for the given flags.  Optionally open may also
 * return an arbitrary filehandle in the fuse_file_info structure,
 * which will be passed to all file operations.
 *
 * Changed in version 2.2
 */
int sfs_open(const char *path, struct fuse_file_info *fi){
  log_msg("\nsfs_open(path\"%s\", fi=0x%08x)\n",
    path, fi);
  int retstat = 0;

  if(getBlock(path) == NULL){
    return -1;
  }
  int flags = fcntl(fi->fh, F_GETFL);
  
  if(flags == -1){
    return -1;
  }
  
  return retstat;
}
/** Release an open file
 *
 * Release is called when there are no more references to an open
 * file: all file descriptors are closed and all memory mappings
 * are unmapped.
 *
 * For every open() call there will be exactly one release() call
 * with the same flags and file descriptor.  It is possible to
 * have a file opened more than once, in which case only the last
 * release will mean, that no more reads/writes will happen on the
 * file.  The return value of release is ignored.
 *
 * Changed in version 2.2
 */
int sfs_release(const char *path, struct fuse_file_info *fi){
  log_msg("\nsfs_release(path=\"%s\", fi=0x%08x)\n",
    path, fi);
  int retstat = 0;

  return retstat;
}

/** Read data from an open file
 *
 * Read should return exactly the number of bytes requested except
 * on EOF or error, otherwise the rest of the data will be
 * substituted with zeroes.  An exception to this is when the
 * 'direct_io' mount option is specified, in which case the return
 * value of the read system call will reflect the return value of
 * this operation.
 *
 * Changed in version 2.2
 */
int sfs_read(const char *path, char *buf, size_t size, off_t offset, struct fuse_file_info *fi){
  log_msg("\nsfs_read(path=\"%s\", buf=0x%08x, size=%d, offset=%lld, fi=0x%08x)\n",
    path, buf, size, offset, fi);
  int retstat = size;

  block* newBlock = (block*)malloc(sizeof(block));

  int i;
  int j = 0;
  int k;
  int firstTime = 1;
  block* ptr;

  if (path[j] == '/')
    j = 1;
  for(i = 1; i < strlen(path); i++){
    char word[i-j];
    if(path[i] == '/'){
      for(k = j; k < i; k++)
        word[k-j] = path[k];
      int foundIt = 0;
      if(firstTime == 1){
        for(k = 0; k < (FileSize/BlockSize); k++){
          block_read(i,newBlock);
          foundIt = 1;
          break;
        }
      }
    }
    else if(i != (strlen(path)-1)){
      for(k=0; k< newBlock->s.st_blocks; k++){
        int found = 0;
        if(newBlock->p[k]->path == word){
          found = 1;
          ptr = newBlock->p[k];
          int a;
          for(a = 0; a < ptr->s.st_blocks; a++)
            ptr->p[a]->type = -1;
        }
        if(found == 1){

        }
      }
    }
  }

  return retstat;
}

/** Write data to an open file
 *
 * Write should return exactly the number of bytes requested
 * except on error.  An exception to this is when the 'direct_io'
 * mount option is specified (see read operation).
 *
 * Changed in version 2.2
 */
int sfs_write(const char *path, const char *buf, size_t size, off_t offset,
       struct fuse_file_info *fi){
  log_msg("\nsfs_write(path=\"%s\", buf=0x%08x, size=%d, offset=%lld, fi=0x%08x)\n",
    path, buf, size, offset, fi);
  int retstat = size;

  return retstat;
}


/** Create a directory */
int sfs_mkdir(const char *path, mode_t mode){
  log_msg("\nsfs_mkdir(path=\"%s\", mode=0%3o)\n",
    path, mode);
  int retstat = 0;

  block* directoryBlock = (block*)malloc(sizeof(block));
  

  return retstat;
}


/** Remove a directory */
int sfs_rmdir(const char *path){
  log_msg("sfs_rmdir(path=\"%s\")\n",
    path);
  int retstat = 0;

  return retstat;
}


/** Open directory
 *
 * This method should check if the open operation is permitted for
 * this  directory
 *
 * Introduced in version 2.3
 */
int sfs_opendir(const char *path, struct fuse_file_info *fi){
  log_msg("\nsfs_opendir(path=\"%s\", fi=0x%08x)\n",
    path, fi);
  int retstat = 0;

  return retstat;
}

/** Read directory
 *
 * This supersedes the old getdir() interface.  New applications
 * should use this.
 *
 * The filesystem may choose between two modes of operation:
 *
 * 1) The readdir implementation ignores the offset parameter, and
 * passes zero to the filler function's offset.  The filler
 * function will not return '1' (unless an error happens), so the
 * whole directory is read in a single readdir operation.  This
 * works just like the old getdir() method.
 *
 * 2) The readdir implementation keeps track of the offsets of the
 * directory entries.  It uses the offset parameter and always
 * passes non-zero offset to the filler function.  When the buffer
 * is full (or an error happens) the filler function will return
 * '1'.
 *
 * Introduced in version 2.3
 */
int sfs_readdir(const char *path, void *buf, fuse_fill_dir_t filler, off_t offset,
         struct fuse_file_info *fi){
 
  //int retstat = 0;
  char* name = malloc(strlen(path)+1);
  name[strlen(path)] = '\0';
  strcpy(name, path);
  char* ptr = strchr(name, '/');
  while(ptr!=NULL){
    if(ptr==&(name[strlen(name)-1]))
      break;
    name = &(ptr[1]);
    ptr = strchr(name, '/');
  }
  if(ptr==&(name[strlen(name)-1]))
    name[strlen(name)-1] = '\0';
  /** Function to add an entry in a readdir() operation
 *
 * @param buf the buffer passed to the readdir() operation
 * @param name the file name of the directory entry
 * @param stat file attributes, can be NULL
 * @param off offset of the next entry or zero
 * @return 1 if buffer is full, zero otherwise
 */
/*typedef int (*fuse_fill_dir_t) (void *buf, const char *name,
        const struct stat *stbuf, off_t off);*/
  int ret = filler(buf, name, NULL, offset);

  free(name);

  return ret;
}

/** Release directory
 *
 * Introduced in version 2.3
 */
int sfs_releasedir(const char *path, struct fuse_file_info *fi){
  int retstat = 0;

  return retstat;
}

struct fuse_operations sfs_oper = {
  .init = sfs_init,
  .destroy = sfs_destroy,

  .getattr = sfs_getattr,
  .create = sfs_create,
  .unlink = sfs_unlink,
  .open = sfs_open,
  .release = sfs_release,
  .read = sfs_read,
  .write = sfs_write,

  .rmdir = sfs_rmdir,
  .mkdir = sfs_mkdir,

  .opendir = sfs_opendir,
  .readdir = sfs_readdir,
  .releasedir = sfs_releasedir
};

void sfs_usage(){
  fprintf(stderr, "usage:  sfs [FUSE and mount options] diskFile mountPoint\n");
  abort();
}

int main(int argc, char *argv[]){
  int fuse_stat;

  // sanity checking on the command line
  if ((argc < 3) || (argv[argc-2][0] == '-') || (argv[argc-1][0] == '-'))
    sfs_usage();

  sfs_data = malloc(sizeof(struct sfs_state));
  if (sfs_data == NULL) {
    perror("main calloc");
    abort();
  }

  // Pull the diskfile and save it in internal data
  sfs_data->diskfile = argv[argc-2];
  argv[argc-2] = argv[argc-1];
  argv[argc-1] = NULL;
  argc--;

  sfs_data->logfile = log_open();
  
  // turn over control to fuse
  fprintf(stderr, "about to call fuse_main, %s \n", sfs_data->diskfile);
  fuse_stat = fuse_main(argc, argv, &sfs_oper, sfs_data);
  fprintf(stderr, "fuse_main returned %d\n", fuse_stat);

  return fuse_stat;
}
