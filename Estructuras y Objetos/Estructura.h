//
// Created by estuardo on 11/02/23.
//

#ifndef P1_MIA_202003894_ESTRUCTURA_H
#define P1_MIA_202003894_ESTRUCTURA_H


#include <time.h>

struct Partition{
    char part_status;
    char part_type;
    char part_fit;
    int part_start;
    int part_s;
    char part_name[16];
};

struct MBR{
    int mbr_tamano;
    time_t mbr_fecha_creacion;
    int mbr_dsk_signature;
    char dsk_fit;
    Partition mbr_partition_[4];
};

struct EBR{
    char part_status;
    char part_fit;
    int part_start;
    int part_s;
    int part_next;
    char part_name [16];
};

struct SuperBloque{
    int s_filesystem_type;
    int s_inodes_count;
    int s_blocks_count;
    int s_free_blocks_count;
    int s_free_inodes_count;
    time_t s_mtime;
    time_t s_umtime;
    int s_mnt_count;
    int s_magic;
    int s_inode_size;
    int s_block_size;
    int s_first_ino;
    int s_first_blo;
    int s_bm_inode_start;
    int s_bm_block_start;
    int s_inode_start;
    int s_block_start;
};

struct TablaInodo{
    int i_uid;
    int i_gid;
    int i_s;
    time_t i_atime;
    time_t i_ctime;
    time_t i_mtime;
    int i_block[15];
    char i_type;
    int i_perm;
};

struct Content{
    char b_name[12];
    int b_inodo;
};

struct BloqueCarpeta{
    Content b_content[4];
};

struct BloqueArchivo{
    char b_content [64];
};

struct BloqueApuntadores{
    int b_pointers [16];
};

struct Journaling{
    char tipo_operacion[10];
    char tipo;
    char path[100];
    char contenido[100];
    time_t fecha;
    int size;
    int sig;
    int start;
};

#endif //P1_MIA_202003894_ESTRUCTURA_H
