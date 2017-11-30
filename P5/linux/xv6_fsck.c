#include <stdio.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <string.h>


//#ifndef _FS_H_
//#define _FS_H_

// On-disk file system format.
// Both the kernel and user programs use this header file.

// Block 0 is unused.
// Block 1 is super block.
// Inodes start at block 2.

#define ROOTINO 1  // root i-number
#define BSIZE 512  // block size

// File system super block
struct superblock {
  uint size;         // Size of file system image (blocks)
  uint nblocks;      // Number of data blocks
  uint ninodes;      // Number of inodes.
};

#define NDIRECT 12
#define NINDIRECT (BSIZE / sizeof(uint))
#define MAXFILE (NDIRECT + NINDIRECT)

// On-disk inode structure
struct dinode {
  short type;           // File type
  short major;          // Major device number (T_DEV only)
  short minor;          // Minor device number (T_DEV only)
  short nlink;          // Number of links to inode in file system
  uint size;            // Size of file (bytes)
  uint addrs[NDIRECT+1];   // Data block addresses
};

// Inodes per block.
#define IPB           (BSIZE / sizeof(struct dinode))

// Block containing inode i
#define IBLOCK(i)     ((i) / IPB + 2)

// Bitmap bits per block
#define BPB           (BSIZE*8)

// Block containing bit for block b
//#define BBLOCK(b, ninodes) (b/BPB + (ninodes)/IPB + 3)

// Directory is a file containing a sequence of dirent structures.
#define DIRSIZ 14

struct dirent {
  ushort inum;
  char name[DIRSIZ];
};

//#endif // _FS_H_


//#ifndef _TYPES_H_
//#define _TYPES_H_

// Type definitions

typedef unsigned int   uint;
typedef unsigned short ushort;
typedef unsigned char  uchar;
typedef uint pde_t;
#ifndef NULL
#define NULL (0)
#endif

//#endif //_TYPES_H_
#define T_FILE 2
#define T_DIR 1
#define T_DEV 3
#define UNUSED 0

void checkInUse (void *image, struct dinode *inode, uint high, uint low) {
  int reachIndir = 0;
  int round = (inode->size-1)/BSIZE + 1;
  if (round > NDIRECT) {
    round = NDIRECT;
    reachIndir = 1;
  }
  int i;
  for (i = 0; i < round; i++) {
  	//printf("addrs is %d\n", inode->addrs[i]);
    if (inode->addrs[i] >= high || inode->addrs[i] < low) {
      fprintf(stderr, "ERROR: bad direct address in inode.\n");
      exit(1);
    }
  }
  if (reachIndir != 1) return;
  uint * indirblock = (uint *)(image + BSIZE*inode->addrs[NDIRECT]);
  round = (inode->size - NDIRECT*BSIZE - 1)/BSIZE + 1;
  if (round > BSIZE/sizeof(uint)) {
    perror("file size larger than max_file_size.\n");
    exit(1);
  }
  for (i = 0; i < round; i++) {
    if (*(uint *)indirblock >= high || *(uint *)indirblock < low) {
      fprintf(stderr, "ERROR: bad indirect address in inode.\n");
      exit(1);
    }
    indirblock++;
  }
  
}

uint checkDirFormat (void *image, struct dinode *inode) {
  // check . and .. exist
  struct dirent *dir = (struct dirent*)(image + BSIZE*inode->addrs[0]);
  if (strcmp(dir->name, ".") != 0) {
    fprintf(stderr, "ERROR: directory not properly formatted.\n");
    exit(1);
  }
  dir++;
  if (strcmp(dir->name, "..") != 0) {
    fprintf(stderr, "ERROR: directory not properly formatted.\n");
    exit(1);
  }
  return dir->inum; // return parent's inum
}


int checkRoot(void * image, struct dinode *inode) {
  struct dirent *dir = (struct dirent *)(image + BSIZE*inode->addrs[0]);
  if (dir->inum == 1) {
  	dir++;
  	if (dir->inum == 1) {
  	  return 1;
  	}
  }
  return 0;
}


void checkChild(void *image, struct dinode *pnode, uint childInum) {
  int i;
  for (i = 0; i < NDIRECT; i++) {
    struct dirent *dir = (struct dirent *)(image + BSIZE*pnode->addrs[i]);
    int j;
    for (j = 0; j < 32; j++) {
      if (dir->inum == childInum) return;
      dir++;
    }
  }
  uint *indiraddr = (uint *)(image + BSIZE*pnode->addrs[NDIRECT]);
  for (i = 0; i < BSIZE/sizeof(uint); i++) {
    struct dirent *dir = (struct dirent *)(image + BSIZE*(*indiraddr));
    int j;
    for (j = 0; j < 32; j++) {
      if (dir->inum == childInum) return;
      dir++;
    }
  	indiraddr++;
  }
 
  fprintf(stderr, "ERROR: parent directory mismatch.\n");
  exit(1);
}


void checkParent(void *image, uint parentInum, uint childInum, uint ninodes) {
  if (parentInum == childInum) {
  	fprintf(stderr, "ERROR: parent directory mismatch.\n");
    exit(1);
  }
  struct dinode *inode = (struct dinode *)(image + 2*BSIZE);
  int i;
  for (i = 0; i < ninodes; i++) {
    if (inode->type == T_DIR) { // only directory type can be parent of others
      struct dirent *currdir = (struct dirent *)(image + BSIZE*inode->addrs[0]);
      uint inum = currdir->inum;
      if (inum == parentInum) { // this is dad, see if he has this child
        checkChild(image, inode, childInum);
        return;
      }
    }
  	inode++;
  }

  fprintf(stderr, "ERROR: parent directory mismatch.\n");
  exit(1);
  
}


int main(int argc, char *argv[]) {
  int fd;

  if (argc != 2) {
    fprintf(stderr, "Usage: xv6_fsck file_system_image\n");
    exit(1);
  }

  fd = open(argv[1], O_RDONLY);
  if (fd < 0) {
    fprintf(stderr, "image not found.\n");
    exit(1);
  }

  struct stat fileInfo;
  if (fstat(fd, &fileInfo) == -1) {
    perror("error geting the file stat\n");
    exit(1);
  }

  if (fileInfo.st_size == 0) {
    perror("file is empty\n");
    exit(1);
  }

  void *image = mmap(NULL, fileInfo.st_size, PROT_READ, MAP_PRIVATE, fd, 0);
  if (image == MAP_FAILED) {
    perror("mmap failed\n");
    exit(1);
  }

  // super block
  struct superblock *sb = (struct superblock *)(image + BSIZE);
  uint sz_image = sb->size;
  uint nblocks = sb->nblocks;
  uint ninodes = sb->ninodes;

  // check inode
  struct dinode *runner = (struct dinode *)(image + 2*BSIZE);
  int i;
  for (i = 0; i < ninodes; i++) {
    if ((runner->type != UNUSED) && (runner->type != T_FILE)
    	&& (runner->type != T_DEV) && (runner->type != T_DIR)) {
      fprintf(stderr, "ERROR: bad inode.\n");
      exit(1);
    }
    // for in-use inodes, check inode
    if (runner->type != UNUSED) {
      //if (runner->size == 0) continue;
      checkInUse(image, runner, sz_image, sz_image-nblocks);
    }
    runner++;    
  }

  // check direct format
  runner = (struct dinode *)(image + 2*BSIZE);
  for (i = 0; i < ninodes; i++) {
  	if (runner->type == T_DIR) {
  	  uint parentInum = checkDirFormat(image, runner);
  	}
    runner++;
  }

  // check root 
  struct dinode *rootnode = (struct dinode*)(image + 2*BSIZE);
  int checkroot = 0;
  for (i = 0; i < ninodes; i++) {
    if (rootnode->type == T_DIR) {
      if ((checkroot = checkRoot(image, rootnode)) == 1) break;
    }
    rootnode++;
  }
  if (checkroot == 0) {
  	fprintf(stderr, "ERROR: root directory does not exist.\n");
    exit(1);
  }

  // check parent
  runner = (struct dinode *)(image + 2*BSIZE);
  for (i = 0; i < ninodes; i++) {
    if (runner->type == T_DIR) {
      struct dirent *dotdir = (struct dirent *)(image + BSIZE*runner->addrs[0]);
      if (dotdir->inum != ROOTINO) { // only check for non-root
      	struct dirent *dotdotdir = dotdir++;
      	checkParent(image, dotdotdir->inum, dotdir->inum, ninodes);
      }
    }
  	runner++;
  }

  // check bit
  //checkBit(img, sb);
  exit(0);
}
/*
void checkBit(void * img, struct superblock *sb) {
  int bit[sb->size];
  memset(bit, 0, sb->size);
  memset(bit, 1, sb->size - sb->nblocks);

  uchar
}*/