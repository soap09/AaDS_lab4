#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "sys/stat.h"


struct file_header {
  int active;
  int deleted;
  int offset_active;
  int offset_first_deleted;
  int offset_last_deleted;
};

struct file_record {
  char is_deleted;
  char name[20];
  int offset_next;
};

int check_file(char filename[]) {
  if (access(filename, F_OK) != 0) {
    return 2;
  }
  FILE * fp = fopen(filename, "rb");
  if (fp == NULL) {
    return -1;
  }
  struct stat statbuf;
  stat(filename, &statbuf);
  struct file_header Header;
  if (statbuf.st_size == 0) return 2;
  fread(&Header, sizeof(struct file_header), 1, fp);
  int records_in_file = (statbuf.st_size - sizeof(struct file_header)) / sizeof(struct file_record);
  fclose(fp);
  if (Header.active + Header.deleted == records_in_file) return 1;
  else if (statbuf.st_size != 0) return 0;
}

int create_empty_file(char filename[]) {
  struct file_header Header = {0, 0, -1, -1, -1};
  FILE * fp = fopen(filename, "wb");
  fwrite(&Header, sizeof(Header), 1, fp);
  if (ferror(fp)) {
    return 0;
  }
  fclose(fp);
  return 1;
}

int insert_record(char filename[], char data[]) {
  // Откроем файл на чтение, считаем заголовок, сохраним смещение первого элемента
  // Откроем файл на дозапись, запишем новый элемент, следующий элемент для нового - бывший первый
  // Вернемся в начало и перезапишем заголовок
  FILE * fp = fopen(filename, "rb");
  struct file_header Header;
  fread(&Header, sizeof(struct file_header), 1, fp);
  
  struct file_record read;
  while (fread(&read, sizeof(struct file_record), 1, fp) == 1) {
    if (strncmp(read.name, data, 20) == 0) return 0;
  }

  fclose(fp);
  

  struct file_record Record;
  Record.is_deleted = 0;
  strcpy(Record.name, data);
  Record.offset_next = Header.offset_active;

  fp = fopen(filename, "ab");

  Header.active = Header.active + 1;
  Header.offset_active = (int) ftell(fp);

  fwrite(&Record, sizeof(struct file_record), 1, fp);
  fclose(fp);
  fp = fopen(filename, "r+b");
  int count = fwrite(&Header, sizeof(struct file_header), 1, fp);
  fclose(fp);
  return 1;
}

int main(int argc, char *argv[]) {
  if (argc != 3) {
    printf("Usage: %s [FILENAME] [DATA]\n", argv[0]);
    return 1;
  }
  char *filename = argv[1]; 
  char *data = argv[2];
  int check_result = check_file(filename);

  if (check_result == 0) {
    perror("Bad File!\n");
    return 1;
  }
  else if (check_result == -1) {
    return 1;
  }
  
  if (check_result == 2) {
    create_empty_file(filename);
  }
  if (!insert_record(filename, data)) {
    printf("%s already in %s, ignoring!\n", data, filename);
    return 1;
  }
  return 0;
}
