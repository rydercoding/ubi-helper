#include <unistd.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/ioctl.h>
#include <string.h>
#include <stdint.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#include <assert.h>
#include "ubifs-media.h"

#define UBIFS_IN_FILE_PATH "./test-master.ubifs"
#define UBIFS_CORRUPT_FILE_PATH "./corrupted.ubifs"

#define LEB_SIZE 0x3E000 /* 248KB */
#define MST_NODE_ALSZ 0x1000
#define FILE_BUF_SZ (4*1024)

static int ubifs_fd = -1;
static int out_fd = -1;

static uint8_t leb_buf[LEB_SIZE];
static uint8_t file_buf[FILE_BUF_SZ];

static void dump_mst_nodes(int file_ofs)
{
  struct ubifs_mst_node mst = {0};
  int mst_size = sizeof(struct ubifs_mst_node);
  int buf_idx = 0;
  int valid_mst_count = 0;

  while (buf_idx < LEB_SIZE)
  {
    memcpy(&mst, &leb_buf[buf_idx], mst_size);
    if (mst.ch.magic == UBIFS_NODE_MAGIC && mst.ch.node_type == UBIFS_MST_NODE)
    {
      valid_mst_count++;
      printf("### valid_mst_count = %d\n", valid_mst_count);
      printf("\tfile offset of this node = 0x%x\n", file_ofs + buf_idx);
    }
    
    buf_idx += MST_NODE_ALSZ;
  }

  /* Dump master#1 node 6*/
  memcpy(&mst, &leb_buf[5 * MST_NODE_ALSZ], mst_size);
  printf("### node 6\n");
  printf("### common header:\n");
  printf("\tmagic = 0x%08x, UBIFS_NODE_MAGIC = 0x%08x\n", mst.ch.magic, UBIFS_NODE_MAGIC);
  printf("\tcrc = 0x%08x\n", mst.ch.crc);
  printf("\tsqnum = %llu\n", mst.ch.sqnum);
  printf("\tlen = %u\n", mst.ch.len);
  printf("\tnode_type = %u, UBIFS_MST_NODE = %u\n", mst.ch.node_type, UBIFS_MST_NODE);
  printf("\tgroup_type = %u\n", mst.ch.group_type);

  printf("### others:\n");
  printf("\thighest_inum = %llu\n", mst.highest_inum);
  printf("\tcmt_no = %llu\n", mst.cmt_no);
  printf("\tflags = 0x%x\n", mst.flags);
  printf("\tlog_lnum = %u\n", mst.log_lnum);
  printf("\troot_lnum = %u\n", mst.root_lnum);
  printf("\troot_offs = %u\n", mst.root_offs);
  printf("\troot_len = %u\n", mst.root_len);
  printf("\tgc_lnum = %u\n", mst.gc_lnum);
  printf("\tihead_lnum = %u\n", mst.ihead_lnum);
  printf("\tihead_offs = %u\n", mst.ihead_offs);
  printf("\tindex_size = %llu\n", mst.index_size);
  printf("\ttotal_free = %llu\n", mst.total_free);
  printf("\ttotal_dirty = %llu\n", mst.total_dirty);
  printf("\ttotal_used = %llu\n", mst.total_used);
  printf("\ttotal_dead = %llu\n", mst.total_dead);
  printf("\ttotal_dark = %llu\n", mst.total_dark);
  printf("\tlpt_lnum = %u\n", mst.lpt_lnum);
  printf("\tlpt_offs = %u\n", mst.lpt_offs);
  printf("\tnhead_lnum = %u\n", mst.nhead_lnum);
  printf("\tltab_lnum = %u\n", mst.ltab_lnum);
  printf("\tltab_offs = %u\n", mst.ltab_offs);
  printf("\tlsave_lnum = %u\n", mst.lsave_lnum);
  printf("\tlsave_offs = %u\n", mst.lsave_offs);
  printf("\tlscan_lnum = %u\n", mst.lscan_lnum);
  printf("\tempty_lebs = %u\n", mst.empty_lebs);
  printf("\tidx_lebs = %u\n", mst.idx_lebs);
  printf("\tleb_cnt = %u\n", mst.leb_cnt);
}

static void dump_super_node(void)
{
  struct ubifs_sb_node super_node = {0};
  int super_size = sizeof(struct ubifs_sb_node);
  int index;

  memcpy(&super_node, &leb_buf[0], super_size);
  printf("### common header:\n");
  printf("\tmagic = 0x%08x, UBIFS_NODE_MAGIC = 0x%08x\n", super_node.ch.magic, UBIFS_NODE_MAGIC);
  printf("\tcrc = 0x%08x\n", super_node.ch.crc);
  printf("\tsqnum = %llu\n", super_node.ch.sqnum);
  printf("\tlen = %u\n", super_node.ch.len);
  printf("\tnode_type = %u, UBIFS_SB_NODE = %u\n", super_node.ch.node_type, UBIFS_SB_NODE);
  printf("\tgroup_type = %u\n", super_node.ch.group_type);
  
  printf("### others:\n");
  printf("\tkey_hash = %u, UBIFS_KEY_HASH_R5 = %u\n", super_node.key_hash, UBIFS_KEY_HASH_R5);
  printf("\tkey_fmt = %u, UBIFS_SIMPLE_KEY_FMT = %u\n", super_node.key_fmt, UBIFS_SIMPLE_KEY_FMT);
  printf("\tflags = 0x%x\n", super_node.flags);
  printf("\tmin_io_size = %uKB(%uu byte)\n", super_node.min_io_size / 1024, super_node.min_io_size);
  printf("\tleb_size = %uKB(%u byte)\n", super_node.leb_size / 1024, super_node.leb_size);
  printf("\tleb_cnt = %u\n", super_node.leb_cnt);
  printf("\tmax_leb_cnt = %u\n", super_node.max_leb_cnt);
  printf("\tmax_bud_bytes = %llu\n", super_node.max_bud_bytes);
  printf("\tlog_lebs = %u(lebs)\n", super_node.log_lebs);
  printf("\tlpt_lebs = %u(lebs)\n", super_node.lpt_lebs);
  printf("\torph_lebs = %u(lebs)\n", super_node.orph_lebs);
  printf("\tjhead_cnt = %u\n", super_node.jhead_cnt);
  printf("\tfanout = %u\n", super_node.fanout);
  printf("\tlsave_cnt = %u(lebs)\n", super_node.lsave_cnt);
  printf("\tfmt_version = %u, UBIFS_FORMAT_VERSION = %u\n", 
         super_node.fmt_version, UBIFS_FORMAT_VERSION);
  printf("\tdefault_compr = %hu\n", super_node.default_compr);
  printf("\trp_uid = %u\n", super_node.rp_uid);
  printf("\trp_gid = %u\n", super_node.rp_gid);
  printf("\trp_size = %llu\n", super_node.rp_size);
  printf("\ttime_gran = %u(nanoseconds)\n", super_node.time_gran);
  for (index = 0; index < 16; index++)
  {
    if (index == 8) printf("\n");
    printf("\tuuid[%d] = 0x%02x", index, super_node.uuid[index]);
  }
  printf("\n");
  printf("\tro_compat_version = %u, UBIFS_RO_COMPAT_VERSION=%u\n", 
          super_node.ro_compat_version, UBIFS_RO_COMPAT_VERSION);
  /* TBD for other hmac fields */
}

static int dump_ubifs(void)
{
  int ret = 0;
  int read_count = 0;
  
  ubifs_fd = open(UBIFS_IN_FILE_PATH, O_RDONLY);
  
  if (ubifs_fd != -1)
  {
    /* Read LEB#0 from the file to the buffer. */
    printf("-------------------- Dump super block --------------------\n");
    memset(leb_buf, 0xBD, LEB_SIZE);
    if (lseek(ubifs_fd, 0, SEEK_SET) != -1)
    {
      read_count = read(ubifs_fd, leb_buf, LEB_SIZE);
      if (read_count == LEB_SIZE)
      {
        dump_super_node();
      }
      else
      {
        ret = EIO;
        printf("File read failed with read_count %d", read_count);
      }
    }
    else
    {
      ret = errno;
      printf("Fail to call lseek for super block\n");
    }

    /* Read LEB#1 from the file to the buffer. */
    printf("-------------------- Dump master#1 --------------------\n");
    memset(leb_buf, 0xBD, LEB_SIZE);
    if (lseek(ubifs_fd, LEB_SIZE, SEEK_SET) != -1)
    {
      read_count = read(ubifs_fd, leb_buf, LEB_SIZE);
      if (read_count == LEB_SIZE)
      {
        dump_mst_nodes(LEB_SIZE);
      }
      else
      {
        ret = EIO;
        printf("File read failed with read_count %d", read_count);
      }
    }
    else
    {
      ret = errno;
      printf("Fail to call lseek for master#1\n");
    }


    /* Read LEB#2 from the file to the buffer. */
    printf("-------------------- Dump master#2 --------------------\n");
    memset(leb_buf, 0xBD, LEB_SIZE);
    if (lseek(ubifs_fd, LEB_SIZE * 2, SEEK_SET) != -1)
    {
      read_count = read(ubifs_fd, leb_buf, LEB_SIZE);
      if (read_count == LEB_SIZE)
      {
        dump_mst_nodes(LEB_SIZE * 2);
      }
      else
      {
        printf("File read failed with read_count %d", read_count);
      }
    }
    else
    {
      ret = errno;
      printf("Fail to call lseek for master#2\n");
    }
  
    (void)close(ubifs_fd);
  }
  else
  {
    ret = errno;
    printf("Fail to open input file\n");
  }

  return ret;
}

static int dup_img_file(void)
{
  int ret = 0;
  int bytes_read = 0;
  int bytes_written = 0;
  
  ubifs_fd = open(UBIFS_IN_FILE_PATH, O_RDONLY);

  if (ubifs_fd != -1)
  {
    out_fd = open(UBIFS_CORRUPT_FILE_PATH, O_RDWR | O_CREAT | O_TRUNC, 0644);
    if (out_fd != -1)
    {
      bytes_read = read(ubifs_fd, file_buf, FILE_BUF_SZ);
      while (bytes_read > 0)
      {
        bytes_written = write(out_fd, file_buf, FILE_BUF_SZ);
        if (bytes_written != bytes_read)
        {
          printf("Fail to write output file\n");
          ret = 1;
          break;
        }

        bytes_read = read(ubifs_fd, file_buf, FILE_BUF_SZ);
      }

      if (bytes_read == -1)
      {
        printf("Fail to read input file\n");
        ret = 1;
      }

      close(out_fd);
    }
    else
    {
      printf("Fail to create output file\n");
      ret = errno;
    }
      
    close(ubifs_fd);
  }
  else
  {
    printf("Fail to open input file\n");
    ret = errno;
  }

  return ret;
}

static bool corrupt_mst_node(void)
{
  struct ubifs_mst_node mst = {0};
  struct ubifs_mst_node corrupt_mst = {0};
  int mst_size = sizeof(struct ubifs_mst_node);
  int buf_idx = 0;
  int last_buf_idx = -1;
  int valid_mst_count = 0;
  bool status = true;

  while (buf_idx < LEB_SIZE)
  {
    memcpy(&mst, &leb_buf[buf_idx], mst_size);
    if (mst.ch.magic == UBIFS_NODE_MAGIC && mst.ch.node_type == UBIFS_MST_NODE)
    {
      valid_mst_count++;
      last_buf_idx = buf_idx;
      
      printf("### valid_mst_count = %d\n", valid_mst_count);
      printf("buf_idx = 0x%x\n", buf_idx);
    }

    buf_idx += MST_NODE_ALSZ;
  } 

  /* Check if there is still space to append another 1 node */
  if (last_buf_idx + MST_NODE_ALSZ + MST_NODE_ALSZ <= LEB_SIZE)
  {
    /* Copy the last node and corrupt it */
    memcpy(&corrupt_mst, &leb_buf[last_buf_idx], sizeof(struct ubifs_mst_node));
    printf("corrupt_mst.ch.magic = 0x%x\n", corrupt_mst.ch.magic);
    printf("#1corrupt_mst.ch.crc = 0x%x\n", corrupt_mst.ch.crc );
    corrupt_mst.ch.crc++; /* Corrupt CRC field */
    printf("#2corrupt_mst.ch.crc = 0x%x\n", corrupt_mst.ch.crc );

    /* Append the corrupted node */
    memcpy(&leb_buf[last_buf_idx + MST_NODE_ALSZ], &corrupt_mst, sizeof(struct ubifs_mst_node));

    printf("(last_buf_idx + MST_NODE_ALSZ)=0x%x\n", last_buf_idx + MST_NODE_ALSZ);
    printf("leb_buf[last_buf_idx + MST_NODE_ALSZ]=0x%x\n", leb_buf[last_buf_idx + MST_NODE_ALSZ]);
  }
  else
  {
    printf("No enough space to write corrupted node\n");
    status = false;
  }

  return status;
}

static int corrupt_master(void)
{
  int read_count = 0;
  int bytes_written = 0;
  int ret = 0;

  /* Read LEB#1 from the file to the buffer. */
  memset(leb_buf, 0xBD, LEB_SIZE);
  if (lseek(out_fd, LEB_SIZE, SEEK_SET) != -1)
  {
    read_count = read(out_fd, leb_buf, LEB_SIZE);
    if (read_count == LEB_SIZE)
    {
      if (corrupt_mst_node())
      {
        /* Write LEB#1 buffer back to the new ubifs image. */
        assert(lseek(out_fd, LEB_SIZE, SEEK_SET) != -1);
        bytes_written = write(out_fd, leb_buf, LEB_SIZE);
        fsync(out_fd);
        printf("Write LEB#1 buffer back to the new ubifs image, bytes_written=0x%x\n", 
                bytes_written);
        if (bytes_written != LEB_SIZE)
        {
          printf("Fail to write output file\n");
          ret = 1;
        }
      }
    }
    else
    {
      printf("File read failed with read_count %d", read_count);
      ret = EIO;
    }
  }
  else
  {
    printf("Fail to call lseek for master#1\n");
    ret = errno;
  }

  return ret;
}

static bool corrupt_super_node(void)
{
  bool status = true;
  struct ubifs_sb_node super_node = {0};
  int super_size = sizeof(struct ubifs_sb_node);

  memcpy(&super_node, &leb_buf[0], super_size);
  printf("node_type = 0x%x, UBIFS_SB_NODE=0x%x\n", super_node.ch.node_type, UBIFS_SB_NODE);
  printf("magic = 0x%x, UBIFS_NODE_MAGIC=0x%x\n", super_node.ch.magic, UBIFS_NODE_MAGIC);
  printf("crc = 0x%x\n", super_node.ch.crc);
  //super_node.ch.magic = 0xbdbdbdbd;
  //super_node.ch.crc = 0xbdbdbdbd;
  super_node.ch.node_type = 0xbd;

  /* Write back the corrupted node to leb buffer */
  memcpy(&leb_buf[0], &super_node, super_size);

  return status;
}

static int corrupt_super(void)
{
  int read_count = 0;
  int bytes_written = 0;
  int ret = 0;

  /* Read LEB#0 from the file to the buffer. */
  memset(leb_buf, 0xBD, LEB_SIZE);
  if (lseek(out_fd, 0, SEEK_SET) != -1)
  {
    read_count = read(out_fd, leb_buf, LEB_SIZE);
    if (read_count == LEB_SIZE)
    {
      if (corrupt_super_node())
      {
        /* Write LEB#0 buffer back to the new ubifs image. */
        assert(lseek(out_fd, 0, SEEK_SET) != -1);
        bytes_written = write(out_fd, leb_buf, LEB_SIZE);
        fsync(out_fd);
        printf("Write LEB#0 buffer back to the new ubifs image, bytes_written=0x%x\n", 
                bytes_written);
        if (bytes_written != LEB_SIZE)
        {
          printf("Fail to write output file\n");
          ret = 1;
        }
      }
    }
    else
    {
      printf("File read failed with read_count %d", read_count);
      ret = EIO;
    }
  }
  else
  {
    printf("Fail to call lseek for master#1\n");
    ret = errno;
  }

  return ret;
}


static int corrupt_ubifs(void)
{
  int ret = 0;
  int read_count = 0;
  int bytes_written = 0;

  ret = dup_img_file();
  
  out_fd = open(UBIFS_CORRUPT_FILE_PATH, O_RDWR);
  if (out_fd != -1)
  {
    //ret = corrupt_master();
    ret = corrupt_super();

    (void)close(out_fd);
  }
  else
  {
    printf("Fail to open file\n");
    ret = errno;
  }

  return ret;
}

int main(int argc, char **argv)
{
  int ret = 0;
  int opt;

  while ((opt = getopt(argc, argv, "dc")) != -1)
  {
    switch (opt)
    {
      case 'd':
        printf("opt is -d\n");
        ret = dump_ubifs();
        break;

      case 'c':
        printf("opt is -c\n");
        ret = corrupt_ubifs();
        break;

      default:
        printf("Unsupported command options\n");
        ret = 1;
        break;
    }
  }
  
  return ret;
}

