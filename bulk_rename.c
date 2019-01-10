#include <stdio.h>
#include <stdlib.h>
#include <dirent.h>
#include <string.h>
#include <time.h>
#if defined(_WIN32)
#include <direct.h>
#else
#include <sys/stat.h>
#endif

#define PATH_PROPERTY_FILE_NAME "paths.props"
#define EXTENSION_PROPERTY_FILE_NAME "extensions.props"
#define TRIAL_LABEL_PROPERTY_FILE_NAME "labels.props"
#define PROPERTY_COMMENT_CHARACTER '#'
#define RENAMED_FOLDER_PREFIX "/RENAMED_"

typedef struct l{
   char **names;
   int count;
} list;

typedef struct t{
  long key;
  char *value;
} tuple;

typedef struct h{
  tuple *elements;
  int size;
} minHeap;

void gatherProperties(list *string_list, char *properties_file);
void renameVideos(list *paths, list *trials, list *extensions);
char *extractExtension(char* fileName);
int intMin(int one, int two);

void makeList(list *l);
void addToList(list *l, char* item);
int contains(list *l, char *item);

void makeHeap(minHeap *h);
void addToHeap(minHeap *h, tuple *tup);

long extractTimestamp(char* file_name);
long stringToInt(char *number_string);
int asciiToInt(char c);
long power(int base, int exp);
int isNumberCharacter(char c);

char *copyString(char *string);
char *concat(char *source, char *destinat);

void makedir(char *name);
int copyFile(char *from, char *to);

int main(int argc, char **argv) {
  list paths, trials, extensions;
  makeList(&paths);
  makeList(&trials);
  makeList(&extensions);
  gatherProperties(&paths, PATH_PROPERTY_FILE_NAME);
  gatherProperties(&trials, TRIAL_LABEL_PROPERTY_FILE_NAME);
  gatherProperties(&extensions, EXTENSION_PROPERTY_FILE_NAME);
  renameVideos(&paths, &trials, &extensions);
  return (EXIT_SUCCESS);
}

void gatherProperties(list *string_list, char *properties_file) {
  int INITIAL_BUFFER_SIZE = 20;
  char *file_buff = NULL;
  char *old_buff;
  FILE *f = fopen(properties_file, "r");
  int partial_line = 0;

  while (!feof(f)) {
    if (file_buff == NULL) {
      file_buff = malloc(sizeof(char) * INITIAL_BUFFER_SIZE);
    }
    fgets(file_buff, INITIAL_BUFFER_SIZE, f);
    if(partial_line) {
      file_buff = concat(file_buff, old_buff);
      int size = strlen(file_buff);
      if (file_buff[size - 1] == '\n') {
        // add pointer to directories struct
        file_buff[size - 1] = '\0';
        if (file_buff[0] != PROPERTY_COMMENT_CHARACTER) {
          addToList(string_list, file_buff);
        }
        file_buff = malloc(sizeof(char) * INITIAL_BUFFER_SIZE);
        partial_line = 0;
      } else {
        old_buff = malloc(sizeof(char) * size + 1);
        strcpy(old_buff, file_buff);
      }
    } else {
      int size = strlen(file_buff);
      if (file_buff[size - 1] != '\n') {
        partial_line = 1;
        old_buff = malloc(sizeof(char) * strlen(file_buff) + 1);
        strcpy(old_buff, file_buff);
      } else {
        // add pointer to directories struct
        file_buff[size - 1] = '\0';
        if (file_buff[0] != PROPERTY_COMMENT_CHARACTER) {
          addToList(string_list, file_buff);
        }
        file_buff = malloc(sizeof(char) * INITIAL_BUFFER_SIZE);
      }
    }
  }
  if (file_buff != NULL && file_buff[0] != PROPERTY_COMMENT_CHARACTER) {
    addToList(string_list, file_buff);
  }
  fclose(f);
}

void renameVideos(list *paths, list *trials, list *extensions) {
  for (int i = 0; i < paths->count; i++) {
    list *filtered_files = malloc(sizeof(list));
    makeList(filtered_files);
    DIR *dir = opendir(*(paths->names + i));
    struct dirent *dp;
    if (!dir){
      printf("Directory %s not found.\n", *(paths->names + i));
    }
    // Go through files and filter out names without the appropriate extension(s)
    while (dir) {
      if ((dp = readdir(dir)) != NULL) {
        char *name = malloc(sizeof(char) * (strlen(dp->d_name) + 1));
        strcpy(name, dp->d_name);
        char *extension = extractExtension(name);
        if (extension != NULL) {
          if(contains(extensions, extension)) {
            addToList(filtered_files, name);
          }
          free(extension);
        }
      } else {
        break;
      }
    }
    if (filtered_files->count != trials->count) {
      printf("Warning, number of trial labels does not equal the number of files in directory %s. There may be misnamed files.\n", *(paths->names + i));
    }
    // go through filtered list, extract timestamp, and add to heap
    minHeap *heap = malloc(sizeof(minHeap));
    makeHeap(heap);
    for (int j = 0; j < filtered_files->count; j++) {
      tuple *entry = malloc(sizeof(tuple));
      entry->key = extractTimestamp(*(filtered_files->names + j));
      entry->value = *(filtered_files->names + j);
      if (entry->key != -1) {
        addToHeap(heap, entry);
      }
    }
    free(filtered_files);
    char *timestamp = malloc(sizeof(char) * 15);
    snprintf(timestamp, 15, "%lld", time(NULL));
    char *prefix = copyString(RENAMED_FOLDER_PREFIX);

    // create folder for renamed files
    char *file_base = copyString(*(paths->names + i));
    char *fully_qualified_folder = concat(concat(timestamp, prefix), file_base);
    makedir(fully_qualified_folder);

    for (int j = 0; j < intMin(heap->size, trials->count); j++) {
      char *base = concat(copyString("/"), copyString(fully_qualified_folder));
      char *new_name = concat(copyString(*((trials->names) + j)), base);
      char *old_name = concat(copyString((*((heap->elements) + j)).value), concat(copyString("/"), copyString(*(paths->names + i))));
      new_name = concat(extractExtension(old_name), concat(copyString("."), new_name));
      copyFile(old_name, new_name);
    }
  }
}

char *extractExtension(char* fileName) {
  int begin = strlen(fileName);
  for (int i = strlen(fileName); i >= 0; i--) {
    if (*(fileName + i) == '.') {
      begin = i + 1;
      break;
    }
  }
  int size = strlen(fileName) - begin;
  if(size <= 0) {
    return NULL;
  }
  char *extension = malloc(sizeof(char) * (size+1));
  for(int i = 0; i < size; i++) {
    *(extension + i) = *(fileName + begin + i);
  }
  *(extension + size) = '\0';
  return extension;
}

int intMin(int one, int two) {
  if (one < two) {
    return one;
  }
  return two;
}

// list functions
void makeList(list *l){
  l->count = 0;
  l->names = NULL;
}

void addToList(list *l, char* item) {
  int count = l->count;
  char **names = l->names;
  char **new_names = malloc(sizeof(char*) * (count + 1));
  for(int i = 0; i < count; i++) {
    *(new_names + i) = *(names + i);
  }
  if (names != NULL) {
    free(names);
  }
  *(new_names + count) = item;
  l->names = new_names;
  l->count++;
}

int contains(list *l, char *item) {
  for (int i = 0; i < l->count; i++) {
    if (!strcmp(item, *(l->names + i))) {
      return 1;
    }
  }
  return 0;
}

// heap functions
void makeHeap(minHeap *h) {
  h->elements = NULL;
  h->size = 0;
}

void addToHeap(minHeap *h, tuple *tup) {
  int key = tup->key;
  int size = h->size;
  tuple *elements = h->elements;
  int start = 0;
  int end = size - 1;
  int position;
  // Find the appropriate position
  while(1) {
    if (start > end) {
      position = start;
      break;
    } else if (start == end) {
      if ( (*(elements + start)).key > tup->key) {
        position = start;
      }  else {
        position = start + 1;
      }
      break;
    } else {
      int feeler = (start + end)/2;
      int feeler_key = (*(elements + feeler)).key;
      if (feeler_key > tup->key) {
        end = feeler - 1;
      } else if (feeler_key < tup->key) {
        start = feeler + 1;
      } else {
        position = feeler;
        break;
      }
    }
  }
  // Inject tuple at position
  tuple *new_elements = malloc(sizeof(tuple) * (size + 1));
  for (int i = 0; i < position; i++) {
    *(new_elements + i) = *(elements + i);
  }

  *(new_elements + position) = *tup;
  for (int i = position; i < size; i++) {
    *(new_elements + i + 1) = *(elements + i);
  }
  if (elements != NULL) {
    free(elements);
  }
  h->elements = new_elements;
  h->size++;
}

long extractTimestamp(char* file_name) {
  int in_number = 0;
  int number_start = -1;
  int number_end = -1;
  for (int i = 0; i < strlen(file_name); i++) {
    if (in_number) {
      // we've seen the first number
      if(!isNumberCharacter(*(file_name + i))) {
        number_end = i;
        break;
      }
    } else {
      // still need to find it
      if(isNumberCharacter(*(file_name + i))){
        number_start = i;
        in_number = 1;
      }
    }
  }
  if (number_start == -1) {
    return -1;
  }
  int size = number_end - number_start;
  char *number_string = malloc(sizeof(char) * (size + 1));

  for (int i = 0; i < size; i++){
    *(number_string + i) = *(file_name + number_start + i);
  }
  *(number_string + size) = '\0';
  long result = stringToInt(number_string);
  free(number_string);
  return result;
}

long stringToInt(char *number_string) {
  long sum = 0L;
  int size = strlen(number_string);
  for (int i = 0 ; i < size; i++) {
    sum += asciiToInt(*(number_string + i)) * power(10, size-1-i);
  }
  return sum;
}

int asciiToInt(char c) {
  switch(c) {
    case '0':
      return 0;
    case '1':
      return 1;
    case '2':
      return 2;
    case '3':
      return 3;
    case '4':
      return 4;
    case '5':
      return 5;
    case '6':
      return 6;
    case '7':
      return 7;
    case '8':
      return 8;
    case '9':
      return 9;
    default:
      return -1;
    }
}

long power(int base, int exp) {
  long sum = 1L;
  for(int i = 0; i < exp; i++) {
    sum *= base;
  }
  return sum;
}

int isNumberCharacter(char c) {
  switch(c) {
    case '0':
    case '1':
    case '2':
    case '3':
    case '4':
    case '5':
    case '6':
    case '7':
    case '8':
    case '9':
      return 1;
    default:
      return 0;
    }
}

char *copyString(char *string) {
  char *memory = malloc(sizeof(char) * (strlen(string) + 1));
  strcpy(memory, string);
  return memory;
}

char *concat(char *source, char *destination) {
  int new_length = strlen(source) + strlen(destination) + 1;
  char *new_buffer = malloc(sizeof(char) * new_length);
  for(int i = 0; i < strlen(destination); i++) {
    *(new_buffer+i) = *(destination+i);
  }
  int offset = strlen(destination);
  for(int i = 0; i < strlen(source); i++) {
    *(new_buffer + i + offset) = *(source + i);
  }
  new_buffer[new_length-1] = '\0';
  free(source);
  free(destination);
  return new_buffer;
}

void makedir(char *name) {
  #if defined(_WIN32)
  _mkdir(name);
  #else
  mkdir(name, 0700);
  #endif
}

int copyFile(char *from, char *to) {
  FILE *from_file = fopen(from, "rb");
  FILE *to_file = fopen(to, "wb");
  unsigned char character = fgetc(from_file);
  while(!feof(from_file)) {
    fputc(character, to_file);
    character = fgetc(from_file);
  }
  fclose(from_file);
  fclose(to_file);
}