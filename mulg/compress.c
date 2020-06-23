/*

  

*/

#include <stdio.h>
#include <stdlib.h>

/* Useful constants. */
#define FALSE 0
#define TRUE  1

/* Global variables. */
FILE *source_file,*dest_file;
int byte_stored_status=FALSE;
int byte_stored_val;

/* Pseudo procedures */
#define end_of_data()  (byte_stored_status?FALSE:!(byte_stored_status=((byte_stored_val=fgetc(source_file))!=EOF)))
#define read_byte()  (byte_stored_status?byte_stored_status=FALSE,(unsigned char)byte_stored_val:(unsigned char)fgetc(source_file))
#define write_byte(byte)  ((void)fputc((byte),dest_file))

unsigned long int val_to_read=0, val_to_write=0;
unsigned char bit_counter_to_read=0, bit_counter_to_write=0;

typedef struct s_dic_val { unsigned int character;
                           unsigned int code;
                           struct s_dic_val *left_ptr,
                                            *right_ptr;
                         } t_dic_val,*p_dic_val;
#define CHAR_DIC_VAL(ptr_dic)  ((*(ptr_dic)).character)
#define CODE_DIC_VAL(ptr_dic)  ((*(ptr_dic)).code)
#define PLEFT_DIC_VAL(ptr_dic)  ((*(ptr_dic)).left_ptr)
#define PRIGHT_DIC_VAL(ptr_dic)  ((*(ptr_dic)).right_ptr)

unsigned int index_dic;
/* Word counter already known in the dictionary. */
unsigned char bit_counter_encoding;
/* Bit counter in the encoding. */

#define EXP2_DIC_MAX  12
unsigned int index_dic_max;
unsigned char input_bit_counter, bit_counter_min_encoding;
unsigned int initialization_code;
unsigned int end_information_code;
p_dic_val dictionary[1<<EXP2_DIC_MAX];

void init_dictionary1()
{ register unsigned int i;

  index_dic_max=1<<12;       /* Attention: Possible values: 2^3 to 2^EXP2_DIC_MAX. */
  input_bit_counter=8;       /* Attention: Possible values: 1 to EXP2_DIC_MAX-1
                                (usually, for pictures, set up to 1, or 4, or 8, in the case
                                of monochrome, or 16-colors, or 256-colors picture). */
  if (input_bit_counter==1)
     bit_counter_min_encoding=3;
  else bit_counter_min_encoding=input_bit_counter+1;
  initialization_code=1<<(bit_counter_min_encoding-1);
  end_information_code=initialization_code-1;
  for (i=0;i<=end_information_code;i++)
      { if ((dictionary[i]=(p_dic_val)malloc(sizeof(t_dic_val)))==NULL)
           { while (i)
             { i--;
               free(dictionary[i]);
             }
             exit(1);
           }
        CHAR_DIC_VAL(dictionary[i])=i;
        CODE_DIC_VAL(dictionary[i])=i;
        PLEFT_DIC_VAL(dictionary[i])=NULL;
        PRIGHT_DIC_VAL(dictionary[i])=NULL;
      }
  for (;i<index_dic_max;i++)
      dictionary[i]=NULL;
  index_dic=end_information_code+1;
  bit_counter_encoding=bit_counter_min_encoding;
}

void init_dictionary2()
{ register unsigned int i;

  for (i=0;i<index_dic_max;i++)
      { PLEFT_DIC_VAL(dictionary[i])=NULL;
        PRIGHT_DIC_VAL(dictionary[i])=NULL;
      }
  index_dic=end_information_code+1;
  bit_counter_encoding=bit_counter_min_encoding;
}

void remove_dictionary() { 
  register unsigned int i;

  for (i=0;(i<index_dic_max)&&(dictionary[i]!=NULL);i++)
      free(dictionary[i]);
}

p_dic_val find_node(current_node,symbol)
p_dic_val current_node;
unsigned int symbol;
{ p_dic_val new_node;

  if (PLEFT_DIC_VAL(current_node)==NULL)
     return current_node;
  else { new_node=PLEFT_DIC_VAL(current_node);
         while ((CHAR_DIC_VAL(new_node)!=symbol)&&(PRIGHT_DIC_VAL(new_node)!=NULL))
               new_node=PRIGHT_DIC_VAL(new_node);
         return new_node;
       }
}

void add_node(current_node,new_node,symbol)
p_dic_val current_node,new_node;
unsigned int symbol;
{ if (dictionary[index_dic]==NULL)
     { if ((dictionary[index_dic]=(p_dic_val)malloc(sizeof(t_dic_val)))==NULL)
          { remove_dictionary();
            exit(1);
          }
       CODE_DIC_VAL(dictionary[index_dic])=index_dic;
       PLEFT_DIC_VAL(dictionary[index_dic])=NULL;
       PRIGHT_DIC_VAL(dictionary[index_dic])=NULL;
     }
  CHAR_DIC_VAL(dictionary[index_dic])=symbol;
  if (current_node==new_node)
     PLEFT_DIC_VAL(new_node)=dictionary[index_dic];
  else PRIGHT_DIC_VAL(new_node)=dictionary[index_dic];
  index_dic++;
  if (index_dic==(1 << bit_counter_encoding))
     bit_counter_encoding++;
}

#define dictionary_sature()  (index_dic==index_dic_max)

void write_code_lr(value)
unsigned int value;
{ val_to_write=(val_to_write << bit_counter_encoding) | value;
  bit_counter_to_write += bit_counter_encoding;
  while (bit_counter_to_write>=8)
        { bit_counter_to_write -= 8;
          write_byte((unsigned char)(val_to_write >> bit_counter_to_write));
          val_to_write &= ((1<< bit_counter_to_write)-1);
        }
}

void complete_encoding_lr()
{ if (bit_counter_to_write>0)
     write_byte((unsigned char)(val_to_write << (8-bit_counter_to_write)));
  val_to_write=bit_counter_to_write=0;
}

void write_code_rl(value)
unsigned int value;
{ val_to_write |= ((unsigned long int)value) << bit_counter_to_write;
  bit_counter_to_write += bit_counter_encoding;
  while (bit_counter_to_write>=8)
        { bit_counter_to_write -= 8;
          write_byte((unsigned char)(val_to_write&0xFF));
          val_to_write = (val_to_write>>8)&((1<< bit_counter_to_write)-1);
        }
}

void complete_encoding_rl()
{ if (bit_counter_to_write>0)
     write_byte((unsigned char)val_to_write);
  val_to_write=bit_counter_to_write=0;
}

unsigned int read_input()
{ unsigned int read_code;

  while (bit_counter_to_read<input_bit_counter)
        { val_to_read=(val_to_read<<8)|read_byte();
          bit_counter_to_read += 8;
        }
  bit_counter_to_read -= input_bit_counter;
  read_code=val_to_read>>bit_counter_to_read;
  val_to_read &= ((1<<bit_counter_to_read)-1);
  return read_code;
}

#define end_input()  ((bit_counter_to_read<input_bit_counter)&&end_of_data())

void encode()
{ p_dic_val current_node,new_node;
  unsigned int symbol;

  if (!end_input())
     { init_dictionary1();
       current_node=dictionary[read_input()];
       while (!end_input())
             { symbol=read_input();
               new_node=find_node(current_node,symbol);
               if ((new_node!=current_node)&&(CHAR_DIC_VAL(new_node)==symbol))
                  current_node=new_node;
               else { write_code_lr(CODE_DIC_VAL(current_node));
                      add_node(current_node,new_node,symbol);
                      if dictionary_sature()
                         {
                           init_dictionary2();
                         }
                      current_node=dictionary[symbol];
                    }
             }
       write_code_lr(CODE_DIC_VAL(current_node));
       complete_encoding_lr();
       remove_dictionary();
     }
}

int main(int argc, char *argv[]) {
  long len;

  if (argc!=3) { exit(1); }
  else if ((source_file = fopen(argv[1],"rb"))==NULL) { exit(1); }
  else if ((dest_file   = fopen(argv[2],"wb"))==NULL) { exit(1); }
  else { 
    /* write original image size */
    fseek(source_file, 0l, SEEK_END);
    len = ftell(source_file);
    fseek(source_file, 0l, SEEK_SET);

    fputc(len>>8, dest_file);
    fputc(len&0xff, dest_file);
    
    encode();
    fclose(source_file);
    fclose(dest_file);
  }
  return (0);
}
