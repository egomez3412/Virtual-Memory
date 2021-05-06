//
//  memmgr.c
//  memmgr
//
//  Created by William McCarthy on 17/11/20.
//  Copyright © 2020 William McCarthy. All rights reserved.
//

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#define ARGC_ERROR 1
#define FILE_ERROR 2
#define BUFLEN 256
#define frame_SIZE  256
#define PAGE_SIZE 256
#define TLB_SIZE 16
#define PHYSICAL_MEMORY_SIZE PAGE_SIZE * frame_SIZE

int address_count = 0; // keeps track of how many addresses have been read
int hit = 0; 
int tlb_size = 0; //size of the tlb
int tlb_hit_count = 0; //used to count how many tlb hits there are
int frame = 0;
int page_fault_count = 0; // counts the amounts of page faults
float page_fault_rate;
float tlb_hit_rate;

//-------------------------------------------------------------------
unsigned getpage(unsigned x) { return (0xff00 & x) >> 8; }

unsigned getoffset(unsigned x) { return (0xff & x); }

void getpage_offset(unsigned x) {
  unsigned  page   = getpage(x);
  unsigned  offset = getoffset(x);
  printf("x is: %u, page: %u, offset: %u, address: %u, paddress: %u\n", x, page, offset,
         (page << 8) | getoffset(x), page * 256 + offset);
}

struct tlb
{
  unsigned int pageNum;
  unsigned int frameNum;
};

int main(int argc, const char* argv[]) {
  FILE* fadd = fopen("addresses.txt", "r");    // open file addresses.txt  (contains the logical addresses)
  if (fadd == NULL) { fprintf(stderr, "Could not open file: 'addresses.txt'\n");  exit(FILE_ERROR);  }

  FILE* fcorr = fopen("correct.txt", "r");     // contains the logical and physical address, and its value
  if (fcorr == NULL) { fprintf(stderr, "Could not open file: 'correct.txt'\n");  exit(FILE_ERROR);  }

  FILE* fbin = fopen("backing_store.bin", "rb"); // Will read the binary storage file
  if (fbin == NULL) { fprintf(stderr, "Could not open file : 'backing_store.bin'\n"); exit(FILE_ERROR); }

  char buf[BUFLEN];
  int physical_memory[PHYSICAL_MEMORY_SIZE];
  unsigned   page, offset, physical_add, frame = 0;
  unsigned   logic_add;                  // read from file address.txt
  unsigned   virt_add, phys_add, value;  // read from file correct.txt

// Declare and initialize page table to -1
  int pageTable[PAGE_SIZE];
  memset(pageTable, -1, 256*sizeof(int));

  //Declare and initialize tlb[] structure to -1
  struct tlb tlb[TLB_SIZE];
  memset(pageTable, -1, 16 * sizeof(char));

  while (fscanf(fadd, "%d", &logic_add) == 1) { // will read every line from address.txt
    address_count++;

    page = getpage(logic_add);
    offset = getoffset(logic_add);
    hit = -1;

    //This will check to see if the page number is already in tlb
    //If it is in tlb, then tlb hit will increase
    for(int i = 0; i < tlb_size; i++)
      if (tlb[i].pageNum == page){
        hit = tlb[i].frameNum;
        physical_add = hit * frame_SIZE + offset;
      }
    if(!(hit == -1))
      tlb_hit_count++;

    //Checking for tlb miss
    //Will get physical page number from table
    else if (pageTable[page] == -1){
      fseek(fbin, page*256, SEEK_SET);
      fread(buf, sizeof(char), BUFLEN, fbin);
      pageTable[page] = frame;

      for(int i = 0; i < 256; i++)
        physical_memory[frame*256 + i] = buf[i];

      page_fault_count++;
      frame++;

      if(tlb_size == 16)
        tlb_size--;

      for(int i = tlb_size; i > 0; i--){
        tlb[i].pageNum = tlb[i - 1].pageNum;
        tlb[i].frameNum = tlb[i - 1].frameNum;
      }

      if(tlb_size <= 15)
        tlb_size++;

      tlb[0].pageNum = page;
      tlb[0].frameNum = pageTable[page];
      physical_add = pageTable[page]*256 + offset;
    }
    else
      physical_add = pageTable[page]*256 + offset;

    fscanf(fcorr, "%s %s %d %s %s %d %s %d", buf, buf, &virt_add, buf, buf, &phys_add, buf, &value);  // read from file correct.txt
    
    assert(physical_add == phys_add);

    // Reads backing_store.bin and confirms that it is the same value in correct.txt 
    fseek(fbin, logic_add, SEEK_SET);
    char c;
    fread(&c, sizeof(char), 1, fbin);
    int val = (int)(c);
    assert(val == value);
    //printf("Value from backing_store.txt is %d whereas value from correct.txt is %d \n", val, value);

    printf("logical: %5u (page: %3u, offset: %3u) ---> physical: %5u -- passed\n", logic_add, page, offset, physical_add);
    if (frame % 5 == 0) { printf("\n"); }

    //The statistics of the program
    page_fault_count = page_fault_count*1.0f / address_count;
    tlb_hit_rate = tlb_hit_count*1.0f /address_count;

  }
  fclose(fbin);
  fclose(fcorr);
  fclose(fadd);

  printf("\nAddresses: %d \n", address_count);
  printf("Page faults: %d \n", page_fault_count);
  printf("Page fault rate: %d \n", page_fault_count);
  printf("TLB hits: %d \n", tlb_hit_count);
  printf("TLB hit rate: %f \n", tlb_hit_rate);
  printf("\n\t\t...done.\n");
  return 0;
}
/////////////////////////////////////////////////////////////////////////
/*
  Output
*/
/////////////////////////////////////////////////////////////////////////
// logical: 60746 (page: 237, offset:  74) ---> physical: 23626 -- passed
// logical: 12199 (page:  47, offset: 167) ---> physical:  2727 -- passed
// logical: 62255 (page: 243, offset:  47) ---> physical: 23855 -- passed
// logical: 21793 (page:  85, offset:  33) ---> physical: 24097 -- passed

// logical: 26544 (page: 103, offset: 176) ---> physical: 24496 -- passed
// logical: 14964 (page:  58, offset: 116) ---> physical: 24692 -- passed
// logical: 41462 (page: 161, offset: 246) ---> physical: 25078 -- passed
// logical: 56089 (page: 219, offset:  25) ---> physical: 25113 -- passed
// logical: 52038 (page: 203, offset:  70) ---> physical: 25414 -- passed

// logical: 47982 (page: 187, offset: 110) ---> physical: 25710 -- passed
// logical: 59484 (page: 232, offset:  92) ---> physical: 25948 -- passed
// logical: 50924 (page: 198, offset: 236) ---> physical: 26348 -- passed
// logical:  6942 (page:  27, offset:  30) ---> physical: 26398 -- passed
// logical: 34998 (page: 136, offset: 182) ---> physical: 26806 -- passed

// logical: 27069 (page: 105, offset: 189) ---> physical: 27069 -- passed
// logical: 51926 (page: 202, offset: 214) ---> physical: 27350 -- passed
// logical: 60645 (page: 236, offset: 229) ---> physical:  7397 -- passed
// logical: 43181 (page: 168, offset: 173) ---> physical: 17581 -- passed
// logical: 10559 (page:  41, offset:  63) ---> physical: 27455 -- passed
// logical:  4664 (page:  18, offset:  56) ---> physical: 27704 -- passed
// logical: 28578 (page: 111, offset: 162) ---> physical: 28066 -- passed

// logical: 59516 (page: 232, offset: 124) ---> physical: 25980 -- passed

// logical: 38912 (page: 152, offset:   0) ---> physical:  3840 -- passed

// logical: 63562 (page: 248, offset:  74) ---> physical: 28234 -- passed
// logical: 64846 (page: 253, offset:  78) ---> physical:  2126 -- passed
// logical: 62938 (page: 245, offset: 218) ---> physical: 28634 -- passed
// logical: 27194 (page: 106, offset:  58) ---> physical: 28730 -- passed
// logical: 28804 (page: 112, offset: 132) ---> physical:  1412 -- passed
// logical: 61703 (page: 241, offset:   7) ---> physical: 22279 -- passed
// logical: 10998 (page:  42, offset: 246) ---> physical: 12534 -- passed
// logical:  6596 (page:  25, offset: 196) ---> physical: 29124 -- passed
// logical: 37721 (page: 147, offset:  89) ---> physical: 29273 -- passed

// logical: 43430 (page: 169, offset: 166) ---> physical: 29606 -- passed
// logical: 22692 (page:  88, offset: 164) ---> physical:  2980 -- passed
// logical: 62971 (page: 245, offset: 251) ---> physical: 28667 -- passed
// logical: 47125 (page: 184, offset:  21) ---> physical: 29717 -- passed
// logical: 52521 (page: 205, offset:  41) ---> physical: 29993 -- passed
// logical: 34646 (page: 135, offset:  86) ---> physical: 18006 -- passed
// logical: 32889 (page: 128, offset: 121) ---> physical:  4217 -- passed
// logical: 13055 (page:  50, offset: 255) ---> physical: 30463 -- passed
// logical: 65416 (page: 255, offset: 136) ---> physical: 30600 -- passed

// logical: 62869 (page: 245, offset: 149) ---> physical: 28565 -- passed

// logical: 57314 (page: 223, offset: 226) ---> physical: 30946 -- passed
// logical: 12659 (page:  49, offset: 115) ---> physical: 31091 -- passed
// logical: 14052 (page:  54, offset: 228) ---> physical: 31460 -- passed
// logical: 32956 (page: 128, offset: 188) ---> physical:  4284 -- passed
// logical: 49273 (page: 192, offset: 121) ---> physical:  8569 -- passed
// logical: 50352 (page: 196, offset: 176) ---> physical: 14000 -- passed
// logical: 49737 (page: 194, offset:  73) ---> physical: 31561 -- passed
// logical: 15555 (page:  60, offset: 195) ---> physical: 31939 -- passed

// logical: 47475 (page: 185, offset: 115) ---> physical: 32115 -- passed
// logical: 15328 (page:  59, offset: 224) ---> physical: 32480 -- passed
// logical: 34621 (page: 135, offset:  61) ---> physical: 17981 -- passed
// logical: 51365 (page: 200, offset: 165) ---> physical: 32677 -- passed
// logical: 32820 (page: 128, offset:  52) ---> physical:  4148 -- passed
// logical: 48855 (page: 190, offset: 215) ---> physical: 32983 -- passed
// logical: 12224 (page:  47, offset: 192) ---> physical:  2752 -- passed
// logical:  2035 (page:   7, offset: 243) ---> physical: 33267 -- passed

// logical: 60539 (page: 236, offset: 123) ---> physical:  7291 -- passed

// logical: 14595 (page:  57, offset:   3) ---> physical: 33283 -- passed
// logical: 13853 (page:  54, offset:  29) ---> physical: 31261 -- passed
// logical: 24143 (page:  94, offset:  79) ---> physical: 33615 -- passed
// logical: 15216 (page:  59, offset: 112) ---> physical: 32368 -- passed
// logical:  8113 (page:  31, offset: 177) ---> physical: 33969 -- passed
// logical: 22640 (page:  88, offset: 112) ---> physical:  2928 -- passed
// logical: 32978 (page: 128, offset: 210) ---> physical:  4306 -- passed
// logical: 39151 (page: 152, offset: 239) ---> physical:  4079 -- passed
// logical: 19520 (page:  76, offset:  64) ---> physical: 22592 -- passed
// logical: 58141 (page: 227, offset:  29) ---> physical: 34077 -- passed
// logical: 63959 (page: 249, offset: 215) ---> physical: 21975 -- passed
// logical: 53040 (page: 207, offset:  48) ---> physical: 34352 -- passed

// logical: 55842 (page: 218, offset:  34) ---> physical: 34594 -- passed
// logical:   585 (page:   2, offset:  73) ---> physical: 17225 -- passed
// logical: 51229 (page: 200, offset:  29) ---> physical: 32541 -- passed
// logical: 64181 (page: 250, offset: 181) ---> physical:  4533 -- passed
// logical: 54879 (page: 214, offset:  95) ---> physical:  3679 -- passed
// logical: 28210 (page: 110, offset:  50) ---> physical: 34866 -- passed
// logical: 10268 (page:  40, offset:  28) ---> physical: 14620 -- passed
// logical: 15395 (page:  60, offset:  35) ---> physical: 31779 -- passed
// logical: 12884 (page:  50, offset:  84) ---> physical: 30292 -- passed
// logical:  2149 (page:   8, offset: 101) ---> physical: 35173 -- passed
// logical: 53483 (page: 208, offset: 235) ---> physical: 35563 -- passed
// logical: 59606 (page: 232, offset: 214) ---> physical: 26070 -- passed
// logical: 14981 (page:  58, offset: 133) ---> physical: 24709 -- passed
// logical: 36672 (page: 143, offset:  64) ---> physical: 35648 -- passed

// logical: 23197 (page:  90, offset: 157) ---> physical: 35997 -- passed
// logical: 36518 (page: 142, offset: 166) ---> physical: 14502 -- passed
// logical: 13361 (page:  52, offset:  49) ---> physical: 36145 -- passed
// logical: 19810 (page:  77, offset:  98) ---> physical: 36450 -- passed
// logical: 25955 (page: 101, offset:  99) ---> physical: 36707 -- passed
// logical: 62678 (page: 244, offset: 214) ---> physical:   470 -- passed
// logical: 26021 (page: 101, offset: 165) ---> physical: 36773 -- passed
// logical: 29409 (page: 114, offset: 225) ---> physical: 37089 -- passed

// logical: 38111 (page: 148, offset: 223) ---> physical: 37343 -- passed
// logical: 58573 (page: 228, offset: 205) ---> physical: 15565 -- passed
// logical: 56840 (page: 222, offset:   8) ---> physical: 37384 -- passed
// logical: 41306 (page: 161, offset:  90) ---> physical: 24922 -- passed
// logical: 54426 (page: 212, offset: 154) ---> physical: 19354 -- passed
// logical:  3617 (page:  14, offset:  33) ---> physical: 10017 -- passed
// logical: 50652 (page: 197, offset: 220) ---> physical: 18652 -- passed
// logical: 41452 (page: 161, offset: 236) ---> physical: 25068 -- passed
// logical: 20241 (page:  79, offset:  17) ---> physical: 16657 -- passed
// logical: 31723 (page: 123, offset: 235) ---> physical: 12267 -- passed
// logical: 53747 (page: 209, offset: 243) ---> physical:  1011 -- passed
// logical: 28550 (page: 111, offset: 134) ---> physical: 28038 -- passed
// logical: 23402 (page:  91, offset: 106) ---> physical: 20586 -- passed
// logical: 21205 (page:  82, offset: 213) ---> physical: 37845 -- passed
// logical: 56181 (page: 219, offset: 117) ---> physical: 25205 -- passed
// logical: 57470 (page: 224, offset: 126) ---> physical: 38014 -- passed
// logical: 39933 (page: 155, offset: 253) ---> physical: 38397 -- passed

// logical: 34964 (page: 136, offset: 148) ---> physical: 26772 -- passed

// logical: 24781 (page:  96, offset: 205) ---> physical: 38605 -- passed
// logical: 41747 (page: 163, offset:  19) ---> physical: 38675 -- passed
// logical: 62564 (page: 244, offset: 100) ---> physical:   356 -- passed
// logical: 58461 (page: 228, offset:  93) ---> physical: 15453 -- passed
// logical: 20858 (page:  81, offset: 122) ---> physical: 39034 -- passed
// logical: 49301 (page: 192, offset: 149) ---> physical:  8597 -- passed
// logical: 40572 (page: 158, offset: 124) ---> physical: 39292 -- passed
// logical: 23840 (page:  93, offset:  32) ---> physical: 39456 -- passed

// logical: 35278 (page: 137, offset: 206) ---> physical: 39886 -- passed
// logical: 62905 (page: 245, offset: 185) ---> physical: 28601 -- passed
// logical: 56650 (page: 221, offset:  74) ---> physical: 13386 -- passed
// logical: 11149 (page:  43, offset: 141) ---> physical: 40077 -- passed
// logical: 38920 (page: 152, offset:   8) ---> physical:  3848 -- passed
// logical: 23430 (page:  91, offset: 134) ---> physical: 20614 -- passed
// logical: 57592 (page: 224, offset: 248) ---> physical: 38136 -- passed
// logical:  3080 (page:  12, offset:   8) ---> physical: 40200 -- passed
// logical:  6677 (page:  26, offset:  21) ---> physical:  6677 -- passed
// logical: 50704 (page: 198, offset:  16) ---> physical: 26128 -- passed
// logical: 51883 (page: 202, offset: 171) ---> physical: 27307 -- passed
// logical: 62799 (page: 245, offset:  79) ---> physical: 28495 -- passed
// logical: 20188 (page:  78, offset: 220) ---> physical: 40668 -- passed
// logical:  1245 (page:   4, offset: 221) ---> physical: 40925 -- passed

// logical: 12220 (page:  47, offset: 188) ---> physical:  2748 -- passed

// logical: 17602 (page:  68, offset: 194) ---> physical: 41154 -- passed
// logical: 28609 (page: 111, offset: 193) ---> physical: 28097 -- passed
// logical: 42694 (page: 166, offset: 198) ---> physical: 20934 -- passed
// logical: 29826 (page: 116, offset: 130) ---> physical: 41346 -- passed
// logical: 13827 (page:  54, offset:   3) ---> physical: 31235 -- passed
// logical: 27336 (page: 106, offset: 200) ---> physical: 28872 -- passed
// logical: 53343 (page: 208, offset:  95) ---> physical: 35423 -- passed
// logical: 11533 (page:  45, offset:  13) ---> physical: 41485 -- passed
// logical: 41713 (page: 162, offset: 241) ---> physical: 41969 -- passed
// logical: 33890 (page: 132, offset:  98) ---> physical: 42082 -- passed

// logical:  4894 (page:  19, offset:  30) ---> physical: 13598 -- passed

// logical: 57599 (page: 224, offset: 255) ---> physical: 38143 -- passed

// logical:  3870 (page:  15, offset:  30) ---> physical: 18974 -- passed

// logical: 58622 (page: 228, offset: 254) ---> physical: 15614 -- passed

// logical: 29780 (page: 116, offset:  84) ---> physical: 41300 -- passed

// logical: 62553 (page: 244, offset:  89) ---> physical:   345 -- passed

// logical:  2303 (page:   8, offset: 255) ---> physical: 35327 -- passed

// logical: 51915 (page: 202, offset: 203) ---> physical: 27339 -- passed

// logical:  6251 (page:  24, offset: 107) ---> physical:  7531 -- passed

// logical: 38107 (page: 148, offset: 219) ---> physical: 37339 -- passed

// logical: 59325 (page: 231, offset: 189) ---> physical: 10429 -- passed

// logical: 61295 (page: 239, offset: 111) ---> physical: 42351 -- passed
// logical: 26699 (page: 104, offset:  75) ---> physical: 42571 -- passed
// logical: 51188 (page: 199, offset: 244) ---> physical: 42996 -- passed
// logical: 59519 (page: 232, offset: 127) ---> physical: 25983 -- passed
// logical:  7345 (page:  28, offset: 177) ---> physical: 43185 -- passed
// logical: 20325 (page:  79, offset: 101) ---> physical: 16741 -- passed
// logical: 39633 (page: 154, offset: 209) ---> physical: 43473 -- passed

// logical:  1562 (page:   6, offset:  26) ---> physical: 43546 -- passed
// logical:  7580 (page:  29, offset: 156) ---> physical:  6300 -- passed
// logical:  8170 (page:  31, offset: 234) ---> physical: 34026 -- passed
// logical: 62256 (page: 243, offset:  48) ---> physical: 23856 -- passed
// logical: 35823 (page: 139, offset: 239) ---> physical: 44015 -- passed
// logical: 27790 (page: 108, offset: 142) ---> physical: 44174 -- passed
// logical: 13191 (page:  51, offset: 135) ---> physical: 44423 -- passed
// logical:  9772 (page:  38, offset:  44) ---> physical: 44588 -- passed

// logical:  7477 (page:  29, offset:  53) ---> physical:  6197 -- passed

// logical: 44455 (page: 173, offset: 167) ---> physical: 44967 -- passed
// logical: 59546 (page: 232, offset: 154) ---> physical: 26010 -- passed
// logical: 49347 (page: 192, offset: 195) ---> physical:  8643 -- passed
// logical: 36539 (page: 142, offset: 187) ---> physical: 14523 -- passed
// logical: 12453 (page:  48, offset: 165) ---> physical: 45221 -- passed
// logical: 49640 (page: 193, offset: 232) ---> physical: 45544 -- passed
// logical: 28290 (page: 110, offset: 130) ---> physical: 34946 -- passed
// logical: 44817 (page: 175, offset:  17) ---> physical: 13073 -- passed
// logical:  8565 (page:  33, offset: 117) ---> physical: 20085 -- passed
// logical: 16399 (page:  64, offset:  15) ---> physical: 45583 -- passed
// logical: 41934 (page: 163, offset: 206) ---> physical: 38862 -- passed
// logical: 45457 (page: 177, offset: 145) ---> physical: 45969 -- passed

// logical: 33856 (page: 132, offset:  64) ---> physical: 42048 -- passed

// logical: 19498 (page:  76, offset:  42) ---> physical: 22570 -- passed

// logical: 17661 (page:  68, offset: 253) ---> physical: 41213 -- passed

// logical: 63829 (page: 249, offset:  85) ---> physical: 21845 -- passed

// logical: 42034 (page: 164, offset:  50) ---> physical: 46130 -- passed
// logical: 28928 (page: 113, offset:   0) ---> physical: 16384 -- passed
// logical: 30711 (page: 119, offset: 247) ---> physical: 16375 -- passed
// logical:  8800 (page:  34, offset:  96) ---> physical: 46432 -- passed
// logical: 52335 (page: 204, offset: 111) ---> physical: 46703 -- passed
// logical: 38775 (page: 151, offset: 119) ---> physical: 46967 -- passed
// logical: 52704 (page: 205, offset: 224) ---> physical: 30176 -- passed
// logical: 24380 (page:  95, offset:  60) ---> physical:  1596 -- passed
// logical: 19602 (page:  76, offset: 146) ---> physical: 22674 -- passed
// logical: 57998 (page: 226, offset: 142) ---> physical:  3214 -- passed
// logical:  2919 (page:  11, offset: 103) ---> physical: 11623 -- passed
// logical:  8362 (page:  32, offset: 170) ---> physical: 47274 -- passed

// logical: 17884 (page:  69, offset: 220) ---> physical:  9948 -- passed

// logical: 45737 (page: 178, offset: 169) ---> physical:  7849 -- passed

// logical: 47894 (page: 187, offset:  22) ---> physical: 25622 -- passed

// logical: 59667 (page: 233, offset:  19) ---> physical: 47379 -- passed
// logical: 10385 (page:  40, offset: 145) ---> physical: 14737 -- passed
// logical: 52782 (page: 206, offset:  46) ---> physical: 47662 -- passed
// logical: 64416 (page: 251, offset: 160) ---> physical:  5024 -- passed
// logical: 40946 (page: 159, offset: 242) ---> physical:  8434 -- passed
// logical: 16778 (page:  65, offset: 138) ---> physical: 48010 -- passed
// logical: 27159 (page: 106, offset:  23) ---> physical: 28695 -- passed
// logical: 24324 (page:  95, offset:   4) ---> physical:  1540 -- passed
// logical: 32450 (page: 126, offset: 194) ---> physical:  7106 -- passed
// logical:  9108 (page:  35, offset: 148) ---> physical: 48276 -- passed
// logical: 65305 (page: 255, offset:  25) ---> physical: 30489 -- passed
// logical: 19575 (page:  76, offset: 119) ---> physical: 22647 -- passed
// logical: 11117 (page:  43, offset: 109) ---> physical: 40045 -- passed
// logical: 65170 (page: 254, offset: 146) ---> physical: 48530 -- passed

// logical: 58013 (page: 226, offset: 157) ---> physical:  3229 -- passed

// logical: 61676 (page: 240, offset: 236) ---> physical: 23276 -- passed

// logical: 63510 (page: 248, offset:  22) ---> physical: 28182 -- passed

// logical: 17458 (page:  68, offset:  50) ---> physical: 41010 -- passed

// logical: 54675 (page: 213, offset: 147) ---> physical: 48787 -- passed
// logical:  1713 (page:   6, offset: 177) ---> physical: 43697 -- passed
// logical: 55105 (page: 215, offset:  65) ---> physical:  5185 -- passed
// logical: 65321 (page: 255, offset:  41) ---> physical: 30505 -- passed
// logical: 45278 (page: 176, offset: 222) ---> physical: 49118 -- passed
// logical: 26256 (page: 102, offset: 144) ---> physical: 49296 -- passed
// logical: 64198 (page: 250, offset: 198) ---> physical:  4550 -- passed
// logical: 29441 (page: 115, offset:   1) ---> physical: 49409 -- passed
// logical:  1928 (page:   7, offset: 136) ---> physical: 33160 -- passed
// logical: 39425 (page: 154, offset:   1) ---> physical: 43265 -- passed
// logical: 32000 (page: 125, offset:   0) ---> physical: 49664 -- passed

// logical: 28549 (page: 111, offset: 133) ---> physical: 28037 -- passed

// logical: 46295 (page: 180, offset: 215) ---> physical: 50135 -- passed
// logical: 22772 (page:  88, offset: 244) ---> physical:  3060 -- passed
// logical: 58228 (page: 227, offset: 116) ---> physical: 34164 -- passed
// logical: 63525 (page: 248, offset:  37) ---> physical: 28197 -- passed
// logical: 32602 (page: 127, offset:  90) ---> physical:  9562 -- passed
// logical: 46195 (page: 180, offset: 115) ---> physical: 50035 -- passed
// logical: 55849 (page: 218, offset:  41) ---> physical: 34601 -- passed
// logical: 46454 (page: 181, offset: 118) ---> physical: 50294 -- passed
// logical:  7487 (page:  29, offset:  63) ---> physical:  6207 -- passed
// logical: 33879 (page: 132, offset:  87) ---> physical: 42071 -- passed
// logical: 42004 (page: 164, offset:  20) ---> physical: 46100 -- passed
// logical:  8599 (page:  33, offset: 151) ---> physical: 20119 -- passed
// logical: 18641 (page:  72, offset: 209) ---> physical:  5585 -- passed
// logical: 49015 (page: 191, offset: 119) ---> physical: 50551 -- passed
// logical: 26830 (page: 104, offset: 206) ---> physical: 42702 -- passed
// logical: 34754 (page: 135, offset: 194) ---> physical: 18114 -- passed
// logical: 14668 (page:  57, offset:  76) ---> physical: 33356 -- passed
// logical: 38362 (page: 149, offset: 218) ---> physical: 20442 -- passed
// logical: 38791 (page: 151, offset: 135) ---> physical: 46983 -- passed
// logical:  4171 (page:  16, offset:  75) ---> physical: 50763 -- passed
// logical: 45975 (page: 179, offset: 151) ---> physical: 51095 -- passed

// logical: 14623 (page:  57, offset:  31) ---> physical: 33311 -- passed

// logical: 62393 (page: 243, offset: 185) ---> physical: 23993 -- passed

// logical: 64658 (page: 252, offset: 146) ---> physical:  6546 -- passed

// logical: 10963 (page:  42, offset: 211) ---> physical: 12499 -- passed

// logical:  9058 (page:  35, offset:  98) ---> physical: 48226 -- passed

// logical: 51031 (page: 199, offset:  87) ---> physical: 42839 -- passed

// logical: 32425 (page: 126, offset: 169) ---> physical:  7081 -- passed

// logical: 45483 (page: 177, offset: 171) ---> physical: 45995 -- passed

// logical: 44611 (page: 174, offset:  67) ---> physical: 11075 -- passed

// logical: 63664 (page: 248, offset: 176) ---> physical: 28336 -- passed

// logical: 54920 (page: 214, offset: 136) ---> physical:  3720 -- passed

// logical:  7663 (page:  29, offset: 239) ---> physical:  6383 -- passed

// logical: 56480 (page: 220, offset: 160) ---> physical: 51360 -- passed
// logical:  1489 (page:   5, offset: 209) ---> physical: 51665 -- passed
// logical: 28438 (page: 111, offset:  22) ---> physical: 27926 -- passed
// logical: 65449 (page: 255, offset: 169) ---> physical: 30633 -- passed
// logical: 12441 (page:  48, offset: 153) ---> physical: 45209 -- passed
// logical: 58530 (page: 228, offset: 162) ---> physical: 15522 -- passed
// logical: 63570 (page: 248, offset:  82) ---> physical: 28242 -- passed
// logical: 26251 (page: 102, offset: 139) ---> physical: 49291 -- passed
// logical: 15972 (page:  62, offset: 100) ---> physical: 21092 -- passed
// logical: 35826 (page: 139, offset: 242) ---> physical: 44018 -- passed
// logical:  5491 (page:  21, offset: 115) ---> physical: 21619 -- passed
// logical: 54253 (page: 211, offset: 237) ---> physical: 51949 -- passed
// logical: 49655 (page: 193, offset: 247) ---> physical: 45559 -- passed
// logical:  5868 (page:  22, offset: 236) ---> physical: 52204 -- passed
// logical: 20163 (page:  78, offset: 195) ---> physical: 40643 -- passed
// logical: 51079 (page: 199, offset: 135) ---> physical: 42887 -- passed
// logical: 21398 (page:  83, offset: 150) ---> physical:  9110 -- passed
// logical: 32756 (page: 127, offset: 244) ---> physical:  9716 -- passed
// logical: 64196 (page: 250, offset: 196) ---> physical:  4548 -- passed
// logical: 43218 (page: 168, offset: 210) ---> physical: 17618 -- passed
// logical: 21583 (page:  84, offset:  79) ---> physical: 52303 -- passed

// logical: 25086 (page:  97, offset: 254) ---> physical: 52734 -- passed
// logical: 45515 (page: 177, offset: 203) ---> physical: 46027 -- passed
// logical: 12893 (page:  50, offset:  93) ---> physical: 30301 -- passed
// logical: 22914 (page:  89, offset: 130) ---> physical: 52866 -- passed
// logical: 58969 (page: 230, offset:  89) ---> physical: 14937 -- passed
// logical: 20094 (page:  78, offset: 126) ---> physical: 40574 -- passed
// logical: 13730 (page:  53, offset: 162) ---> physical: 53154 -- passed
// logical: 44059 (page: 172, offset:  27) ---> physical: 53275 -- passed
// logical: 28931 (page: 113, offset:   3) ---> physical: 16387 -- passed
// logical: 13533 (page:  52, offset: 221) ---> physical: 36317 -- passed
// logical: 33134 (page: 129, offset: 110) ---> physical: 53614 -- passed

// logical: 28483 (page: 111, offset:  67) ---> physical: 27971 -- passed

// logical:  1220 (page:   4, offset: 196) ---> physical: 40900 -- passed

// logical: 38174 (page: 149, offset:  30) ---> physical: 20254 -- passed

// logical: 53502 (page: 208, offset: 254) ---> physical: 35582 -- passed

// logical: 43328 (page: 169, offset:  64) ---> physical: 29504 -- passed

// logical:  4970 (page:  19, offset: 106) ---> physical: 13674 -- passed

// logical:  8090 (page:  31, offset: 154) ---> physical: 33946 -- passed

// logical:  2661 (page:  10, offset: 101) ---> physical: 53861 -- passed
// logical: 53903 (page: 210, offset: 143) ---> physical: 54159 -- passed
// logical: 11025 (page:  43, offset:  17) ---> physical: 39953 -- passed
// logical: 26627 (page: 104, offset:   3) ---> physical: 42499 -- passed
// logical: 18117 (page:  70, offset: 197) ---> physical: 18885 -- passed
// logical: 14505 (page:  56, offset: 169) ---> physical:  5801 -- passed
// logical: 61528 (page: 240, offset:  88) ---> physical: 23128 -- passed
// logical: 20423 (page:  79, offset: 199) ---> physical: 16839 -- passed
// logical: 26962 (page: 105, offset:  82) ---> physical: 26962 -- passed
// logical: 36392 (page: 142, offset:  40) ---> physical: 14376 -- passed
// logical: 11365 (page:  44, offset: 101) ---> physical: 54373 -- passed
// logical: 50882 (page: 198, offset: 194) ---> physical: 26306 -- passed
// logical: 41668 (page: 162, offset: 196) ---> physical: 41924 -- passed
// logical: 30497 (page: 119, offset:  33) ---> physical: 16161 -- passed
// logical: 36216 (page: 141, offset: 120) ---> physical: 54648 -- passed
// logical:  5619 (page:  21, offset: 243) ---> physical: 21747 -- passed
// logical: 36983 (page: 144, offset: 119) ---> physical: 18295 -- passed
// logical: 59557 (page: 232, offset: 165) ---> physical: 26021 -- passed
// logical: 36663 (page: 143, offset:  55) ---> physical: 35639 -- passed
// logical: 36436 (page: 142, offset:  84) ---> physical: 14420 -- passed
// logical: 37057 (page: 144, offset: 193) ---> physical: 18369 -- passed
// logical: 23585 (page:  92, offset:  33) ---> physical: 54817 -- passed

// logical: 58791 (page: 229, offset: 167) ---> physical: 55207 -- passed
// logical: 46666 (page: 182, offset:  74) ---> physical: 55370 -- passed
// logical: 64475 (page: 251, offset: 219) ---> physical:  5083 -- passed
// logical: 21615 (page:  84, offset: 111) ---> physical: 52335 -- passed
// logical: 41090 (page: 160, offset: 130) ---> physical:  8834 -- passed
// logical:  1771 (page:   6, offset: 235) ---> physical: 43755 -- passed
// logical: 47513 (page: 185, offset: 153) ---> physical: 32153 -- passed
// logical: 39338 (page: 153, offset: 170) ---> physical: 55722 -- passed
// logical:  1390 (page:   5, offset: 110) ---> physical: 51566 -- passed
// logical: 38772 (page: 151, offset: 116) ---> physical: 46964 -- passed
// logical: 58149 (page: 227, offset:  37) ---> physical: 34085 -- passed
// logical:  7196 (page:  28, offset:  28) ---> physical: 43036 -- passed
// logical:  9123 (page:  35, offset: 163) ---> physical: 48291 -- passed
// logical:  7491 (page:  29, offset:  67) ---> physical:  6211 -- passed
// logical: 62616 (page: 244, offset: 152) ---> physical:   408 -- passed
// logical: 15436 (page:  60, offset:  76) ---> physical: 31820 -- passed
// logical: 17491 (page:  68, offset:  83) ---> physical: 41043 -- passed
// logical: 53656 (page: 209, offset: 152) ---> physical:   920 -- passed
// logical: 26449 (page: 103, offset:  81) ---> physical: 24401 -- passed
// logical: 34935 (page: 136, offset: 119) ---> physical: 26743 -- passed
// logical: 19864 (page:  77, offset: 152) ---> physical: 36504 -- passed
// logical: 51388 (page: 200, offset: 188) ---> physical: 32700 -- passed
// logical: 15155 (page:  59, offset:  51) ---> physical: 32307 -- passed
// logical: 64775 (page: 253, offset:   7) ---> physical:  2055 -- passed
// logical: 47969 (page: 187, offset:  97) ---> physical: 25697 -- passed
// logical: 16315 (page:  63, offset: 187) ---> physical: 55995 -- passed
// logical:  1342 (page:   5, offset:  62) ---> physical: 51518 -- passed
// logical: 51185 (page: 199, offset: 241) ---> physical: 42993 -- passed
// logical:  6043 (page:  23, offset: 155) ---> physical:  9371 -- passed
// logical: 21398 (page:  83, offset: 150) ---> physical:  9110 -- passed
// logical:  3273 (page:  12, offset: 201) ---> physical: 40393 -- passed
// logical:  9370 (page:  36, offset: 154) ---> physical: 22938 -- passed
// logical: 35463 (page: 138, offset: 135) ---> physical: 56199 -- passed

// logical: 28205 (page: 110, offset:  45) ---> physical: 34861 -- passed

// logical:  2351 (page:   9, offset:  47) ---> physical:  4655 -- passed

// logical: 28999 (page: 113, offset:  71) ---> physical: 16455 -- passed

// logical: 47699 (page: 186, offset:  83) ---> physical: 56403 -- passed
// logical: 46870 (page: 183, offset:  22) ---> physical: 19734 -- passed
// logical: 22311 (page:  87, offset:  39) ---> physical: 11303 -- passed
// logical: 22124 (page:  86, offset: 108) ---> physical: 56684 -- passed
// logical: 22427 (page:  87, offset: 155) ---> physical: 11419 -- passed
// logical: 49344 (page: 192, offset: 192) ---> physical:  8640 -- passed
// logical: 23224 (page:  90, offset: 184) ---> physical: 36024 -- passed
// logical:  5514 (page:  21, offset: 138) ---> physical: 21642 -- passed
// logical: 20504 (page:  80, offset:  24) ---> physical: 56856 -- passed
// logical:   376 (page:   1, offset: 120) ---> physical: 57208 -- passed
// logical:  2014 (page:   7, offset: 222) ---> physical: 33246 -- passed
// logical: 38700 (page: 151, offset:  44) ---> physical: 46892 -- passed
// logical: 13098 (page:  51, offset:  42) ---> physical: 44330 -- passed
// logical: 62435 (page: 243, offset: 227) ---> physical: 24035 -- passed
// logical: 48046 (page: 187, offset: 174) ---> physical: 25774 -- passed
// logical: 63464 (page: 247, offset: 232) ---> physical: 17128 -- passed
// logical: 12798 (page:  49, offset: 254) ---> physical: 31230 -- passed
// logical: 51178 (page: 199, offset: 234) ---> physical: 42986 -- passed
// logical:  8627 (page:  33, offset: 179) ---> physical: 20147 -- passed
// logical: 27083 (page: 105, offset: 203) ---> physical: 27083 -- passed
// logical: 47198 (page: 184, offset:  94) ---> physical: 29790 -- passed
// logical: 44021 (page: 171, offset: 245) ---> physical: 57589 -- passed

// logical: 32792 (page: 128, offset:  24) ---> physical:  4120 -- passed

// logical: 43996 (page: 171, offset: 220) ---> physical: 57564 -- passed

// logical: 41126 (page: 160, offset: 166) ---> physical:  8870 -- passed

// logical: 64244 (page: 250, offset: 244) ---> physical:  4596 -- passed

// logical: 37047 (page: 144, offset: 183) ---> physical: 18359 -- passed

// logical: 60281 (page: 235, offset: 121) ---> physical: 57721 -- passed
// logical: 52904 (page: 206, offset: 168) ---> physical: 47784 -- passed
// logical:  7768 (page:  30, offset:  88) ---> physical: 57944 -- passed
// logical: 55359 (page: 216, offset:  63) ---> physical: 58175 -- passed
// logical:  3230 (page:  12, offset: 158) ---> physical: 40350 -- passed
// logical: 44813 (page: 175, offset:  13) ---> physical: 13069 -- passed
// logical:  4116 (page:  16, offset:  20) ---> physical: 50708 -- passed
// logical: 65222 (page: 254, offset: 198) ---> physical: 48582 -- passed
// logical: 28083 (page: 109, offset: 179) ---> physical:  3507 -- passed
// logical: 60660 (page: 236, offset: 244) ---> physical:  7412 -- passed
// logical:    39 (page:   0, offset:  39) ---> physical: 58407 -- passed
// logical:   328 (page:   1, offset:  72) ---> physical: 57160 -- passed
// logical: 47868 (page: 186, offset: 252) ---> physical: 56572 -- passed
// logical: 13009 (page:  50, offset: 209) ---> physical: 30417 -- passed
// logical: 22378 (page:  87, offset: 106) ---> physical: 11370 -- passed
// logical: 39304 (page: 153, offset: 136) ---> physical: 55688 -- passed
// logical: 11171 (page:  43, offset: 163) ---> physical: 40099 -- passed
// logical:  8079 (page:  31, offset: 143) ---> physical: 33935 -- passed
// logical: 52879 (page: 206, offset: 143) ---> physical: 47759 -- passed
// logical:  5123 (page:  20, offset:   3) ---> physical: 15107 -- passed
// logical:  4356 (page:  17, offset:   4) ---> physical: 58628 -- passed

// logical: 45745 (page: 178, offset: 177) ---> physical:  7857 -- passed

// logical: 32952 (page: 128, offset: 184) ---> physical:  4280 -- passed

// logical:  4657 (page:  18, offset:  49) ---> physical: 27697 -- passed

// logical: 24142 (page:  94, offset:  78) ---> physical: 33614 -- passed

// logical: 23319 (page:  91, offset:  23) ---> physical: 20503 -- passed

// logical: 13607 (page:  53, offset:  39) ---> physical: 53031 -- passed

// logical: 46304 (page: 180, offset: 224) ---> physical: 50144 -- passed

// logical: 17677 (page:  69, offset:  13) ---> physical:  9741 -- passed

// logical: 59691 (page: 233, offset:  43) ---> physical: 47403 -- passed

// logical: 50967 (page: 199, offset:  23) ---> physical: 42775 -- passed

// logical:  7817 (page:  30, offset: 137) ---> physical: 57993 -- passed

// logical:  8545 (page:  33, offset:  97) ---> physical: 20065 -- passed

// logical: 55297 (page: 216, offset:   1) ---> physical: 58113 -- passed

// logical: 52954 (page: 206, offset: 218) ---> physical: 47834 -- passed

// logical: 39720 (page: 155, offset:  40) ---> physical: 38184 -- passed

// logical: 18455 (page:  72, offset:  23) ---> physical:  5399 -- passed

// logical: 30349 (page: 118, offset: 141) ---> physical: 59021 -- passed
// logical: 63270 (page: 247, offset:  38) ---> physical: 16934 -- passed
// logical: 27156 (page: 106, offset:  20) ---> physical: 28692 -- passed
// logical: 20614 (page:  80, offset: 134) ---> physical: 56966 -- passed
// logical: 19372 (page:  75, offset: 172) ---> physical: 14252 -- passed
// logical: 48689 (page: 190, offset:  49) ---> physical: 32817 -- passed
// logical: 49386 (page: 192, offset: 234) ---> physical:  8682 -- passed
// logical: 50584 (page: 197, offset: 152) ---> physical: 18584 -- passed
// logical: 51936 (page: 202, offset: 224) ---> physical: 27360 -- passed
// logical: 34705 (page: 135, offset: 145) ---> physical: 18065 -- passed
// logical: 13653 (page:  53, offset:  85) ---> physical: 53077 -- passed
// logical: 50077 (page: 195, offset: 157) ---> physical: 59293 -- passed
// logical: 54518 (page: 212, offset: 246) ---> physical: 19446 -- passed
// logical: 41482 (page: 162, offset:  10) ---> physical: 41738 -- passed
// logical:  4169 (page:  16, offset:  73) ---> physical: 50761 -- passed
// logical: 36118 (page: 141, offset:  22) ---> physical: 54550 -- passed
// logical:  9584 (page:  37, offset: 112) ---> physical: 59504 -- passed
// logical: 18490 (page:  72, offset:  58) ---> physical:  5434 -- passed
// logical: 55420 (page: 216, offset: 124) ---> physical: 58236 -- passed
// logical:  5708 (page:  22, offset:  76) ---> physical: 52044 -- passed
// logical: 23506 (page:  91, offset: 210) ---> physical: 20690 -- passed
// logical: 15391 (page:  60, offset:  31) ---> physical: 31775 -- passed
// logical: 36368 (page: 142, offset:  16) ---> physical: 14352 -- passed
// logical: 38976 (page: 152, offset:  64) ---> physical:  3904 -- passed
// logical: 50406 (page: 196, offset: 230) ---> physical: 14054 -- passed
// logical: 49236 (page: 192, offset:  84) ---> physical:  8532 -- passed
// logical: 65035 (page: 254, offset:  11) ---> physical: 48395 -- passed
// logical: 30120 (page: 117, offset: 168) ---> physical:   680 -- passed
// logical: 62551 (page: 244, offset:  87) ---> physical:   343 -- passed
// logical: 46809 (page: 182, offset: 217) ---> physical: 55513 -- passed
// logical: 21687 (page:  84, offset: 183) ---> physical: 52407 -- passed
// logical: 53839 (page: 210, offset:  79) ---> physical: 54095 -- passed
// logical:  2098 (page:   8, offset:  50) ---> physical: 35122 -- passed
// logical: 12364 (page:  48, offset:  76) ---> physical: 45132 -- passed
// logical: 45366 (page: 177, offset:  54) ---> physical: 45878 -- passed
// logical: 50437 (page: 197, offset:   5) ---> physical: 18437 -- passed
// logical: 36675 (page: 143, offset:  67) ---> physical: 35651 -- passed
// logical: 55382 (page: 216, offset:  86) ---> physical: 58198 -- passed
// logical: 11846 (page:  46, offset:  70) ---> physical: 59718 -- passed
// logical: 49127 (page: 191, offset: 231) ---> physical: 50663 -- passed
// logical: 19900 (page:  77, offset: 188) ---> physical: 36540 -- passed
// logical: 20554 (page:  80, offset:  74) ---> physical: 56906 -- passed
// logical: 19219 (page:  75, offset:  19) ---> physical: 14099 -- passed
// logical: 51483 (page: 201, offset:  27) ---> physical: 15899 -- passed
// logical: 58090 (page: 226, offset: 234) ---> physical:  3306 -- passed
// logical: 39074 (page: 152, offset: 162) ---> physical:  4002 -- passed
// logical: 16060 (page:  62, offset: 188) ---> physical: 21180 -- passed
// logical: 10447 (page:  40, offset: 207) ---> physical: 14799 -- passed
// logical: 54169 (page: 211, offset: 153) ---> physical: 51865 -- passed
// logical: 20634 (page:  80, offset: 154) ---> physical: 56986 -- passed
// logical: 57555 (page: 224, offset: 211) ---> physical: 38099 -- passed
// logical: 61210 (page: 239, offset:  26) ---> physical: 42266 -- passed
// logical:   269 (page:   1, offset:  13) ---> physical: 57101 -- passed
// logical: 33154 (page: 129, offset: 130) ---> physical: 53634 -- passed
// logical: 64487 (page: 251, offset: 231) ---> physical:  5095 -- passed
// logical: 61223 (page: 239, offset:  39) ---> physical: 42279 -- passed
// logical: 47292 (page: 184, offset: 188) ---> physical: 29884 -- passed
// logical: 21852 (page:  85, offset:  92) ---> physical: 24156 -- passed
// logical:  5281 (page:  20, offset: 161) ---> physical: 15265 -- passed
// logical: 45912 (page: 179, offset:  88) ---> physical: 51032 -- passed
// logical: 32532 (page: 127, offset:  20) ---> physical:  9492 -- passed
// logical: 63067 (page: 246, offset:  91) ---> physical: 22107 -- passed
// logical: 41683 (page: 162, offset: 211) ---> physical: 41939 -- passed
// logical: 20981 (page:  81, offset: 245) ---> physical: 39157 -- passed
// logical: 33881 (page: 132, offset:  89) ---> physical: 42073 -- passed
// logical: 41785 (page: 163, offset:  57) ---> physical: 38713 -- passed
// logical:  4580 (page:  17, offset: 228) ---> physical: 58852 -- passed
// logical: 41389 (page: 161, offset: 173) ---> physical: 25005 -- passed
// logical: 28572 (page: 111, offset: 156) ---> physical: 28060 -- passed
// logical:   782 (page:   3, offset:  14) ---> physical:  7950 -- passed
// logical: 30273 (page: 118, offset:  65) ---> physical: 58945 -- passed
// logical: 62267 (page: 243, offset:  59) ---> physical: 23867 -- passed
// logical: 17922 (page:  70, offset:   2) ---> physical: 18690 -- passed
// logical: 63238 (page: 247, offset:   6) ---> physical: 16902 -- passed
// logical:  3308 (page:  12, offset: 236) ---> physical: 40428 -- passed
// logical: 26545 (page: 103, offset: 177) ---> physical: 24497 -- passed
// logical: 44395 (page: 173, offset: 107) ---> physical: 44907 -- passed
// logical: 39120 (page: 152, offset: 208) ---> physical:  4048 -- passed
// logical: 21706 (page:  84, offset: 202) ---> physical: 52426 -- passed
// logical:  7144 (page:  27, offset: 232) ---> physical: 26600 -- passed
// logical: 30244 (page: 118, offset:  36) ---> physical: 58916 -- passed
// logical:  3725 (page:  14, offset: 141) ---> physical: 10125 -- passed
// logical: 54632 (page: 213, offset: 104) ---> physical: 48744 -- passed
// logical: 30574 (page: 119, offset: 110) ---> physical: 16238 -- passed
// logical:  8473 (page:  33, offset:  25) ---> physical: 19993 -- passed
// logical: 12386 (page:  48, offset:  98) ---> physical: 45154 -- passed
// logical: 41114 (page: 160, offset: 154) ---> physical:  8858 -- passed
// logical: 57930 (page: 226, offset:  74) ---> physical:  3146 -- passed
// logical: 15341 (page:  59, offset: 237) ---> physical: 32493 -- passed
// logical: 15598 (page:  60, offset: 238) ---> physical: 31982 -- passed
// logical: 59922 (page: 234, offset:  18) ---> physical: 10514 -- passed
// logical: 18226 (page:  71, offset:  50) ---> physical:  2354 -- passed
// logical: 48162 (page: 188, offset:  34) ---> physical: 17698 -- passed
// logical: 41250 (page: 161, offset:  34) ---> physical: 24866 -- passed
// logical:  1512 (page:   5, offset: 232) ---> physical: 51688 -- passed
// logical:  2546 (page:   9, offset: 242) ---> physical:  4850 -- passed
// logical: 41682 (page: 162, offset: 210) ---> physical: 41938 -- passed
// logical:   322 (page:   1, offset:  66) ---> physical: 57154 -- passed
// logical:   880 (page:   3, offset: 112) ---> physical:  8048 -- passed
// logical: 20891 (page:  81, offset: 155) ---> physical: 39067 -- passed
// logical: 56604 (page: 221, offset:  28) ---> physical: 13340 -- passed
// logical: 40166 (page: 156, offset: 230) ---> physical:  1254 -- passed
// logical: 26791 (page: 104, offset: 167) ---> physical: 42663 -- passed
// logical: 44560 (page: 174, offset:  16) ---> physical: 11024 -- passed
// logical: 38698 (page: 151, offset:  42) ---> physical: 46890 -- passed
// logical: 64127 (page: 250, offset: 127) ---> physical:  4479 -- passed
// logical: 15028 (page:  58, offset: 180) ---> physical: 24756 -- passed
// logical: 38669 (page: 151, offset:  13) ---> physical: 46861 -- passed
// logical: 45637 (page: 178, offset:  69) ---> physical:  7749 -- passed
// logical: 43151 (page: 168, offset: 143) ---> physical: 17551 -- passed
// logical:  9465 (page:  36, offset: 249) ---> physical: 23033 -- passed
// logical:  2498 (page:   9, offset: 194) ---> physical:  4802 -- passed
// logical: 13978 (page:  54, offset: 154) ---> physical: 31386 -- passed
// logical: 16326 (page:  63, offset: 198) ---> physical: 56006 -- passed
// logical: 51442 (page: 200, offset: 242) ---> physical: 32754 -- passed
// logical: 34845 (page: 136, offset:  29) ---> physical: 26653 -- passed
// logical: 63667 (page: 248, offset: 179) ---> physical: 28339 -- passed
// logical: 39370 (page: 153, offset: 202) ---> physical: 55754 -- passed
// logical: 55671 (page: 217, offset: 119) ---> physical: 60023 -- passed

// logical: 64496 (page: 251, offset: 240) ---> physical:  5104 -- passed

// logical:  7767 (page:  30, offset:  87) ---> physical: 57943 -- passed

// logical:  6283 (page:  24, offset: 139) ---> physical:  7563 -- passed

// logical: 55884 (page: 218, offset:  76) ---> physical: 34636 -- passed

// logical: 61103 (page: 238, offset: 175) ---> physical:  6063 -- passed

// logical: 10184 (page:  39, offset: 200) ---> physical: 60360 -- passed
// logical: 39543 (page: 154, offset: 119) ---> physical: 43383 -- passed
// logical:  9555 (page:  37, offset:  83) ---> physical: 59475 -- passed
// logical: 13963 (page:  54, offset: 139) ---> physical: 31371 -- passed
// logical: 58975 (page: 230, offset:  95) ---> physical: 14943 -- passed
// logical: 19537 (page:  76, offset:  81) ---> physical: 22609 -- passed
// logical:  6101 (page:  23, offset: 213) ---> physical:  9429 -- passed
// logical: 41421 (page: 161, offset: 205) ---> physical: 25037 -- passed
// logical: 45502 (page: 177, offset: 190) ---> physical: 46014 -- passed
// logical: 29328 (page: 114, offset: 144) ---> physical: 37008 -- passed
// logical:  8149 (page:  31, offset: 213) ---> physical: 34005 -- passed
// logical: 25450 (page:  99, offset: 106) ---> physical: 60522 -- passed
// logical: 58944 (page: 230, offset:  64) ---> physical: 14912 -- passed
// logical: 50666 (page: 197, offset: 234) ---> physical: 18666 -- passed
// logical: 23084 (page:  90, offset:  44) ---> physical: 35884 -- passed
// logical: 36468 (page: 142, offset: 116) ---> physical: 14452 -- passed
// logical: 33645 (page: 131, offset: 109) ---> physical: 60781 -- passed
// logical: 25002 (page:  97, offset: 170) ---> physical: 52650 -- passed
// logical: 53715 (page: 209, offset: 211) ---> physical:   979 -- passed
// logical: 60173 (page: 235, offset:  13) ---> physical: 57613 -- passed
// logical: 46354 (page: 181, offset:  18) ---> physical: 50194 -- passed
// logical:  4708 (page:  18, offset: 100) ---> physical: 27748 -- passed
// logical: 28208 (page: 110, offset:  48) ---> physical: 34864 -- passed
// logical: 58844 (page: 229, offset: 220) ---> physical: 55260 -- passed
// logical: 22173 (page:  86, offset: 157) ---> physical: 56733 -- passed
// logical:  8535 (page:  33, offset:  87) ---> physical: 20055 -- passed
// logical: 42261 (page: 165, offset:  21) ---> physical: 10773 -- passed
// logical: 29687 (page: 115, offset: 247) ---> physical: 49655 -- passed
// logical: 37799 (page: 147, offset: 167) ---> physical: 29351 -- passed
// logical: 22566 (page:  88, offset:  38) ---> physical:  2854 -- passed
// logical: 62520 (page: 244, offset:  56) ---> physical:   312 -- passed
// logical:  4098 (page:  16, offset:   2) ---> physical: 50690 -- passed
// logical: 47999 (page: 187, offset: 127) ---> physical: 25727 -- passed
// logical: 49660 (page: 193, offset: 252) ---> physical: 45564 -- passed
// logical: 37063 (page: 144, offset: 199) ---> physical: 18375 -- passed
// logical: 41856 (page: 163, offset: 128) ---> physical: 38784 -- passed
// logical:  5417 (page:  21, offset:  41) ---> physical: 21545 -- passed
// logical: 48856 (page: 190, offset: 216) ---> physical: 32984 -- passed
// logical: 10682 (page:  41, offset: 186) ---> physical: 27578 -- passed
// logical: 22370 (page:  87, offset:  98) ---> physical: 11362 -- passed
// logical: 63281 (page: 247, offset:  49) ---> physical: 16945 -- passed
// logical: 62452 (page: 243, offset: 244) ---> physical: 24052 -- passed
// logical: 50532 (page: 197, offset: 100) ---> physical: 18532 -- passed
// logical:  9022 (page:  35, offset:  62) ---> physical: 48190 -- passed
// logical: 59300 (page: 231, offset: 164) ---> physical: 10404 -- passed
// logical: 58660 (page: 229, offset:  36) ---> physical: 55076 -- passed
// logical: 56401 (page: 220, offset:  81) ---> physical: 51281 -- passed
// logical:  8518 (page:  33, offset:  70) ---> physical: 20038 -- passed
// logical: 63066 (page: 246, offset:  90) ---> physical: 22106 -- passed
// logical: 63250 (page: 247, offset:  18) ---> physical: 16914 -- passed
// logical: 48592 (page: 189, offset: 208) ---> physical:  2000 -- passed
// logical: 28771 (page: 112, offset:  99) ---> physical:  1379 -- passed
// logical: 37673 (page: 147, offset:  41) ---> physical: 29225 -- passed
// logical: 60776 (page: 237, offset: 104) ---> physical: 23656 -- passed
// logical: 56438 (page: 220, offset: 118) ---> physical: 51318 -- passed
// logical: 60424 (page: 236, offset:   8) ---> physical:  7176 -- passed
// logical: 39993 (page: 156, offset:  57) ---> physical:  1081 -- passed
// logical: 56004 (page: 218, offset: 196) ---> physical: 34756 -- passed
// logical: 59002 (page: 230, offset: 122) ---> physical: 14970 -- passed
// logical: 33982 (page: 132, offset: 190) ---> physical: 42174 -- passed
// logical: 25498 (page:  99, offset: 154) ---> physical: 60570 -- passed
// logical: 57047 (page: 222, offset: 215) ---> physical: 37591 -- passed
// logical:  1401 (page:   5, offset: 121) ---> physical: 51577 -- passed
// logical: 15130 (page:  59, offset:  26) ---> physical: 32282 -- passed
// logical: 42960 (page: 167, offset: 208) ---> physical: 19664 -- passed
// logical: 61827 (page: 241, offset: 131) ---> physical: 22403 -- passed
// logical: 32442 (page: 126, offset: 186) ---> physical:  7098 -- passed
// logical: 64304 (page: 251, offset:  48) ---> physical:  4912 -- passed
// logical: 30273 (page: 118, offset:  65) ---> physical: 58945 -- passed
// logical: 38082 (page: 148, offset: 194) ---> physical: 37314 -- passed
// logical: 22404 (page:  87, offset: 132) ---> physical: 11396 -- passed
// logical:  3808 (page:  14, offset: 224) ---> physical: 10208 -- passed
// logical: 16883 (page:  65, offset: 243) ---> physical: 48115 -- passed
// logical: 23111 (page:  90, offset:  71) ---> physical: 35911 -- passed
// logical: 62417 (page: 243, offset: 209) ---> physical: 24017 -- passed
// logical: 60364 (page: 235, offset: 204) ---> physical: 57804 -- passed
// logical:  4542 (page:  17, offset: 190) ---> physical: 58814 -- passed
// logical: 14829 (page:  57, offset: 237) ---> physical: 33517 -- passed
// logical: 44964 (page: 175, offset: 164) ---> physical: 13220 -- passed
// logical: 33924 (page: 132, offset: 132) ---> physical: 42116 -- passed
// logical:  2141 (page:   8, offset:  93) ---> physical: 35165 -- passed
// logical: 19245 (page:  75, offset:  45) ---> physical: 14125 -- passed
// logical: 47168 (page: 184, offset:  64) ---> physical: 29760 -- passed
// logical: 24048 (page:  93, offset: 240) ---> physical: 39664 -- passed
// logical:  1022 (page:   3, offset: 254) ---> physical:  8190 -- passed
// logical: 23075 (page:  90, offset:  35) ---> physical: 35875 -- passed
// logical: 24888 (page:  97, offset:  56) ---> physical: 52536 -- passed
// logical: 49247 (page: 192, offset:  95) ---> physical:  8543 -- passed
// logical:  4900 (page:  19, offset:  36) ---> physical: 13604 -- passed
// logical: 22656 (page:  88, offset: 128) ---> physical:  2944 -- passed
// logical: 34117 (page: 133, offset:  69) ---> physical: 60997 -- passed
// logical: 55555 (page: 217, offset:   3) ---> physical: 59907 -- passed
// logical: 48947 (page: 191, offset:  51) ---> physical: 50483 -- passed
// logical: 59533 (page: 232, offset: 141) ---> physical: 25997 -- passed
// logical: 21312 (page:  83, offset:  64) ---> physical:  9024 -- passed
// logical: 21415 (page:  83, offset: 167) ---> physical:  9127 -- passed
// logical:   813 (page:   3, offset:  45) ---> physical:  7981 -- passed
// logical: 19419 (page:  75, offset: 219) ---> physical: 14299 -- passed
// logical:  1999 (page:   7, offset: 207) ---> physical: 33231 -- passed
// logical: 20155 (page:  78, offset: 187) ---> physical: 40635 -- passed
// logical: 21521 (page:  84, offset:  17) ---> physical: 52241 -- passed
// logical: 13670 (page:  53, offset: 102) ---> physical: 53094 -- passed
// logical: 19289 (page:  75, offset:  89) ---> physical: 14169 -- passed
// logical: 58483 (page: 228, offset: 115) ---> physical: 15475 -- passed
// logical: 41318 (page: 161, offset: 102) ---> physical: 24934 -- passed
// logical: 16151 (page:  63, offset:  23) ---> physical: 55831 -- passed
// logical: 13611 (page:  53, offset:  43) ---> physical: 53035 -- passed
// logical: 21514 (page:  84, offset:  10) ---> physical: 52234 -- passed
// logical: 13499 (page:  52, offset: 187) ---> physical: 36283 -- passed
// logical: 45583 (page: 178, offset:  15) ---> physical:  7695 -- passed
// logical: 49013 (page: 191, offset: 117) ---> physical: 50549 -- passed
// logical: 64843 (page: 253, offset:  75) ---> physical:  2123 -- passed
// logical: 63485 (page: 247, offset: 253) ---> physical: 17149 -- passed
// logical: 38697 (page: 151, offset:  41) ---> physical: 46889 -- passed
// logical: 59188 (page: 231, offset:  52) ---> physical: 10292 -- passed
// logical: 24593 (page:  96, offset:  17) ---> physical: 38417 -- passed
// logical: 57641 (page: 225, offset:  41) ---> physical: 61225 -- passed

// logical: 36524 (page: 142, offset: 172) ---> physical: 14508 -- passed

// logical: 56980 (page: 222, offset: 148) ---> physical: 37524 -- passed

// logical: 36810 (page: 143, offset: 202) ---> physical: 35786 -- passed

// logical:  6096 (page:  23, offset: 208) ---> physical:  9424 -- passed

// logical: 11070 (page:  43, offset:  62) ---> physical: 39998 -- passed

// logical: 60124 (page: 234, offset: 220) ---> physical: 10716 -- passed

// logical: 37576 (page: 146, offset: 200) ---> physical: 21448 -- passed

// logical: 15096 (page:  58, offset: 248) ---> physical: 24824 -- passed

// logical: 45247 (page: 176, offset: 191) ---> physical: 49087 -- passed

// logical: 32783 (page: 128, offset:  15) ---> physical:  4111 -- passed

// logical: 58390 (page: 228, offset:  22) ---> physical: 15382 -- passed

// logical: 60873 (page: 237, offset: 201) ---> physical: 23753 -- passed

// logical: 23719 (page:  92, offset: 167) ---> physical: 54951 -- passed

// logical: 24385 (page:  95, offset:  65) ---> physical:  1601 -- passed

// logical: 22307 (page:  87, offset:  35) ---> physical: 11299 -- passed

// logical: 17375 (page:  67, offset: 223) ---> physical: 61663 -- passed
// logical: 15990 (page:  62, offset: 118) ---> physical: 21110 -- passed
// logical: 20526 (page:  80, offset:  46) ---> physical: 56878 -- passed
// logical: 25904 (page: 101, offset:  48) ---> physical: 36656 -- passed
// logical: 42224 (page: 164, offset: 240) ---> physical: 46320 -- passed
// logical:  9311 (page:  36, offset:  95) ---> physical: 22879 -- passed
// logical:  7862 (page:  30, offset: 182) ---> physical: 58038 -- passed
// logical:  3835 (page:  14, offset: 251) ---> physical: 10235 -- passed
// logical: 30535 (page: 119, offset:  71) ---> physical: 16199 -- passed
// logical: 65179 (page: 254, offset: 155) ---> physical: 48539 -- passed
// logical: 57387 (page: 224, offset:  43) ---> physical: 37931 -- passed
// logical: 63579 (page: 248, offset:  91) ---> physical: 28251 -- passed
// logical:  4946 (page:  19, offset:  82) ---> physical: 13650 -- passed
// logical:  9037 (page:  35, offset:  77) ---> physical: 48205 -- passed
// logical: 61033 (page: 238, offset: 105) ---> physical:  5993 -- passed
// logical: 55543 (page: 216, offset: 247) ---> physical: 58359 -- passed
// logical: 50361 (page: 196, offset: 185) ---> physical: 14009 -- passed
// logical:  6480 (page:  25, offset:  80) ---> physical: 29008 -- passed
// logical: 14042 (page:  54, offset: 218) ---> physical: 31450 -- passed
// logical: 21531 (page:  84, offset:  27) ---> physical: 52251 -- passed
// logical: 39195 (page: 153, offset:  27) ---> physical: 55579 -- passed
// logical: 37511 (page: 146, offset: 135) ---> physical: 21383 -- passed
// logical: 23696 (page:  92, offset: 144) ---> physical: 54928 -- passed
// logical: 27440 (page: 107, offset:  48) ---> physical: 15664 -- passed
// logical: 28201 (page: 110, offset:  41) ---> physical: 34857 -- passed
// logical: 23072 (page:  90, offset:  32) ---> physical: 35872 -- passed
// logical:  7814 (page:  30, offset: 134) ---> physical: 57990 -- passed
// logical:  6552 (page:  25, offset: 152) ---> physical: 29080 -- passed
// logical: 43637 (page: 170, offset: 117) ---> physical: 12661 -- passed
// logical: 35113 (page: 137, offset:  41) ---> physical: 39721 -- passed
// logical: 34890 (page: 136, offset:  74) ---> physical: 26698 -- passed
// logical: 61297 (page: 239, offset: 113) ---> physical: 42353 -- passed
// logical: 45633 (page: 178, offset:  65) ---> physical:  7745 -- passed
// logical: 61431 (page: 239, offset: 247) ---> physical: 42487 -- passed
// logical: 46032 (page: 179, offset: 208) ---> physical: 51152 -- passed
// logical: 18774 (page:  73, offset:  86) ---> physical: 61782 -- passed
// logical: 62991 (page: 246, offset:  15) ---> physical: 22031 -- passed
// logical: 28059 (page: 109, offset: 155) ---> physical:  3483 -- passed
// logical: 35229 (page: 137, offset: 157) ---> physical: 39837 -- passed
// logical: 51230 (page: 200, offset:  30) ---> physical: 32542 -- passed
// logical: 14405 (page:  56, offset:  69) ---> physical:  5701 -- passed
// logical: 52242 (page: 204, offset:  18) ---> physical: 46610 -- passed
// logical: 43153 (page: 168, offset: 145) ---> physical: 17553 -- passed
// logical:  2709 (page:  10, offset: 149) ---> physical: 53909 -- passed
// logical: 47963 (page: 187, offset:  91) ---> physical: 25691 -- passed
// logical: 36943 (page: 144, offset:  79) ---> physical: 18255 -- passed
// logical: 54066 (page: 211, offset:  50) ---> physical: 51762 -- passed
// logical: 10054 (page:  39, offset:  70) ---> physical: 60230 -- passed
// logical: 43051 (page: 168, offset:  43) ---> physical: 17451 -- passed
// logical: 11525 (page:  45, offset:   5) ---> physical: 41477 -- passed
// logical: 17684 (page:  69, offset:  20) ---> physical:  9748 -- passed
// logical: 41681 (page: 162, offset: 209) ---> physical: 41937 -- passed
// logical: 27883 (page: 108, offset: 235) ---> physical: 44267 -- passed
// logical: 56909 (page: 222, offset:  77) ---> physical: 37453 -- passed
// logical: 45772 (page: 178, offset: 204) ---> physical:  7884 -- passed
// logical: 27496 (page: 107, offset: 104) ---> physical: 15720 -- passed
// logical: 46842 (page: 182, offset: 250) ---> physical: 55546 -- passed
// logical: 38734 (page: 151, offset:  78) ---> physical: 46926 -- passed
// logical: 28972 (page: 113, offset:  44) ---> physical: 16428 -- passed
// logical: 59684 (page: 233, offset:  36) ---> physical: 47396 -- passed
// logical: 11384 (page:  44, offset: 120) ---> physical: 54392 -- passed
// logical: 21018 (page:  82, offset:  26) ---> physical: 37658 -- passed
// logical:  2192 (page:   8, offset: 144) ---> physical: 35216 -- passed
// logical: 18384 (page:  71, offset: 208) ---> physical:  2512 -- passed
// logical: 13464 (page:  52, offset: 152) ---> physical: 36248 -- passed
// logical: 31018 (page: 121, offset:  42) ---> physical: 61994 -- passed
// logical: 62958 (page: 245, offset: 238) ---> physical: 28654 -- passed
// logical: 30611 (page: 119, offset: 147) ---> physical: 16275 -- passed
// logical:  1913 (page:   7, offset: 121) ---> physical: 33145 -- passed
// logical: 18904 (page:  73, offset: 216) ---> physical: 61912 -- passed
// logical: 26773 (page: 104, offset: 149) ---> physical: 42645 -- passed
// logical: 55491 (page: 216, offset: 195) ---> physical: 58307 -- passed
// logical: 21899 (page:  85, offset: 139) ---> physical: 24203 -- passed
// logical: 64413 (page: 251, offset: 157) ---> physical:  5021 -- passed
// logical: 47134 (page: 184, offset:  30) ---> physical: 29726 -- passed
// logical: 23172 (page:  90, offset: 132) ---> physical: 35972 -- passed
// logical:  7262 (page:  28, offset:  94) ---> physical: 43102 -- passed
// logical: 12705 (page:  49, offset: 161) ---> physical: 31137 -- passed
// logical:  7522 (page:  29, offset:  98) ---> physical:  6242 -- passed
// logical: 58815 (page: 229, offset: 191) ---> physical: 55231 -- passed
// logical: 34916 (page: 136, offset: 100) ---> physical: 26724 -- passed
// logical:  3802 (page:  14, offset: 218) ---> physical: 10202 -- passed
// logical: 58008 (page: 226, offset: 152) ---> physical:  3224 -- passed
// logical:  1239 (page:   4, offset: 215) ---> physical: 40919 -- passed
// logical: 63947 (page: 249, offset: 203) ---> physical: 21963 -- passed
// logical:   381 (page:   1, offset: 125) ---> physical: 57213 -- passed
// logical: 60734 (page: 237, offset:  62) ---> physical: 23614 -- passed
// logical: 48769 (page: 190, offset: 129) ---> physical: 32897 -- passed
// logical: 41938 (page: 163, offset: 210) ---> physical: 38866 -- passed
// logical: 38025 (page: 148, offset: 137) ---> physical: 37257 -- passed
// logical: 55099 (page: 215, offset:  59) ---> physical:  5179 -- passed
// logical: 56691 (page: 221, offset: 115) ---> physical: 13427 -- passed
// logical: 39530 (page: 154, offset: 106) ---> physical: 43370 -- passed
// logical: 59003 (page: 230, offset: 123) ---> physical: 14971 -- passed
// logical:  6029 (page:  23, offset: 141) ---> physical:  9357 -- passed
// logical: 20920 (page:  81, offset: 184) ---> physical: 39096 -- passed
// logical:  8077 (page:  31, offset: 141) ---> physical: 33933 -- passed
// logical: 42633 (page: 166, offset: 137) ---> physical: 20873 -- passed
// logical: 17443 (page:  68, offset:  35) ---> physical: 40995 -- passed
// logical: 53570 (page: 209, offset:  66) ---> physical:   834 -- passed
// logical: 22833 (page:  89, offset:  49) ---> physical: 52785 -- passed
// logical:  3782 (page:  14, offset: 198) ---> physical: 10182 -- passed
// logical: 47758 (page: 186, offset: 142) ---> physical: 56462 -- passed
// logical: 22136 (page:  86, offset: 120) ---> physical: 56696 -- passed
// logical: 22427 (page:  87, offset: 155) ---> physical: 11419 -- passed
// logical: 23867 (page:  93, offset:  59) ---> physical: 39483 -- passed
// logical: 59968 (page: 234, offset:  64) ---> physical: 10560 -- passed
// logical: 62166 (page: 242, offset: 214) ---> physical: 62422 -- passed
// logical:  6972 (page:  27, offset:  60) ---> physical: 26428 -- passed
// logical: 63684 (page: 248, offset: 196) ---> physical: 28356 -- passed
// logical: 46388 (page: 181, offset:  52) ---> physical: 50228 -- passed
// logical: 41942 (page: 163, offset: 214) ---> physical: 38870 -- passed
// logical: 36524 (page: 142, offset: 172) ---> physical: 14508 -- passed
// logical:  9323 (page:  36, offset: 107) ---> physical: 22891 -- passed
// logical: 31114 (page: 121, offset: 138) ---> physical: 62090 -- passed
// logical: 22345 (page:  87, offset:  73) ---> physical: 11337 -- passed
// logical: 46463 (page: 181, offset: 127) ---> physical: 50303 -- passed
// logical: 54671 (page: 213, offset: 143) ---> physical: 48783 -- passed
// logical:  9214 (page:  35, offset: 254) ---> physical: 48382 -- passed
// logical:  7257 (page:  28, offset:  89) ---> physical: 43097 -- passed
// logical: 33150 (page: 129, offset: 126) ---> physical: 53630 -- passed
// logical: 41565 (page: 162, offset:  93) ---> physical: 41821 -- passed
// logical: 26214 (page: 102, offset: 102) ---> physical: 49254 -- passed
// logical:  3595 (page:  14, offset:  11) ---> physical:  9995 -- passed
// logical: 17932 (page:  70, offset:  12) ---> physical: 18700 -- passed
// logical: 34660 (page: 135, offset: 100) ---> physical: 18020 -- passed
// logical: 51961 (page: 202, offset: 249) ---> physical: 27385 -- passed
// logical: 58634 (page: 229, offset:  10) ---> physical: 55050 -- passed
// logical: 57990 (page: 226, offset: 134) ---> physical:  3206 -- passed
// logical: 28848 (page: 112, offset: 176) ---> physical:  1456 -- passed
// logical: 49920 (page: 195, offset:   0) ---> physical: 59136 -- passed
// logical: 18351 (page:  71, offset: 175) ---> physical:  2479 -- passed
// logical: 53669 (page: 209, offset: 165) ---> physical:   933 -- passed
// logical: 33996 (page: 132, offset: 204) ---> physical: 42188 -- passed
// logical:  6741 (page:  26, offset:  85) ---> physical:  6741 -- passed
// logical: 64098 (page: 250, offset:  98) ---> physical:  4450 -- passed
// logical:   606 (page:   2, offset:  94) ---> physical: 17246 -- passed
// logical: 27383 (page: 106, offset: 247) ---> physical: 28919 -- passed
// logical: 63140 (page: 246, offset: 164) ---> physical: 22180 -- passed
// logical: 32228 (page: 125, offset: 228) ---> physical: 49892 -- passed
// logical: 63437 (page: 247, offset: 205) ---> physical: 17101 -- passed
// logical: 29085 (page: 113, offset: 157) ---> physical: 16541 -- passed
// logical: 65080 (page: 254, offset:  56) ---> physical: 48440 -- passed
// logical: 38753 (page: 151, offset:  97) ---> physical: 46945 -- passed
// logical: 16041 (page:  62, offset: 169) ---> physical: 21161 -- passed
// logical:  9041 (page:  35, offset:  81) ---> physical: 48209 -- passed
// logical: 42090 (page: 164, offset: 106) ---> physical: 46186 -- passed
// logical: 46388 (page: 181, offset:  52) ---> physical: 50228 -- passed
// logical: 63650 (page: 248, offset: 162) ---> physical: 28322 -- passed
// logical: 36636 (page: 143, offset:  28) ---> physical: 35612 -- passed
// logical: 21947 (page:  85, offset: 187) ---> physical: 24251 -- passed
// logical: 19833 (page:  77, offset: 121) ---> physical: 36473 -- passed
// logical: 36464 (page: 142, offset: 112) ---> physical: 14448 -- passed
// logical:  8541 (page:  33, offset:  93) ---> physical: 20061 -- passed
// logical: 12712 (page:  49, offset: 168) ---> physical: 31144 -- passed
// logical: 48955 (page: 191, offset:  59) ---> physical: 50491 -- passed
// logical: 39206 (page: 153, offset:  38) ---> physical: 55590 -- passed
// logical: 15578 (page:  60, offset: 218) ---> physical: 31962 -- passed
// logical: 49205 (page: 192, offset:  53) ---> physical:  8501 -- passed
// logical:  7731 (page:  30, offset:  51) ---> physical: 57907 -- passed
// logical: 43046 (page: 168, offset:  38) ---> physical: 17446 -- passed
// logical: 60498 (page: 236, offset:  82) ---> physical:  7250 -- passed
// logical:  9237 (page:  36, offset:  21) ---> physical: 22805 -- passed
// logical: 47706 (page: 186, offset:  90) ---> physical: 56410 -- passed
// logical: 43973 (page: 171, offset: 197) ---> physical: 57541 -- passed
// logical: 42008 (page: 164, offset:  24) ---> physical: 46104 -- passed
// logical: 27460 (page: 107, offset:  68) ---> physical: 15684 -- passed
// logical: 24999 (page:  97, offset: 167) ---> physical: 52647 -- passed
// logical: 51933 (page: 202, offset: 221) ---> physical: 27357 -- passed
// logical: 34070 (page: 133, offset:  22) ---> physical: 60950 -- passed
// logical: 65155 (page: 254, offset: 131) ---> physical: 48515 -- passed
// logical: 59955 (page: 234, offset:  51) ---> physical: 10547 -- passed
// logical:  9277 (page:  36, offset:  61) ---> physical: 22845 -- passed
// logical: 20420 (page:  79, offset: 196) ---> physical: 16836 -- passed
// logical: 44860 (page: 175, offset:  60) ---> physical: 13116 -- passed
// logical: 50992 (page: 199, offset:  48) ---> physical: 42800 -- passed
// logical: 10583 (page:  41, offset:  87) ---> physical: 27479 -- passed
// logical: 57751 (page: 225, offset: 151) ---> physical: 61335 -- passed
// logical: 23195 (page:  90, offset: 155) ---> physical: 35995 -- passed
// logical: 27227 (page: 106, offset:  91) ---> physical: 28763 -- passed
// logical: 42816 (page: 167, offset:  64) ---> physical: 19520 -- passed
// logical: 58219 (page: 227, offset: 107) ---> physical: 34155 -- passed
// logical: 37606 (page: 146, offset: 230) ---> physical: 21478 -- passed
// logical: 18426 (page:  71, offset: 250) ---> physical:  2554 -- passed
// logical: 21238 (page:  82, offset: 246) ---> physical: 37878 -- passed
// logical: 11983 (page:  46, offset: 207) ---> physical: 59855 -- passed
// logical: 48394 (page: 189, offset:  10) ---> physical:  1802 -- passed
// logical: 11036 (page:  43, offset:  28) ---> physical: 39964 -- passed
// logical: 30557 (page: 119, offset:  93) ---> physical: 16221 -- passed
// logical: 23453 (page:  91, offset: 157) ---> physical: 20637 -- passed
// logical: 49847 (page: 194, offset: 183) ---> physical: 31671 -- passed
// logical: 30032 (page: 117, offset:  80) ---> physical:   592 -- passed
// logical: 48065 (page: 187, offset: 193) ---> physical: 25793 -- passed
// logical:  6957 (page:  27, offset:  45) ---> physical: 26413 -- passed
// logical:  2301 (page:   8, offset: 253) ---> physical: 35325 -- passed
// logical:  7736 (page:  30, offset:  56) ---> physical: 57912 -- passed
// logical: 31260 (page: 122, offset:  28) ---> physical: 23324 -- passed
// logical: 17071 (page:  66, offset: 175) ---> physical:   175 -- passed
// logical:  8940 (page:  34, offset: 236) ---> physical: 46572 -- passed
// logical:  9929 (page:  38, offset: 201) ---> physical: 44745 -- passed
// logical: 45563 (page: 177, offset: 251) ---> physical: 46075 -- passed
// logical: 12107 (page:  47, offset:  75) ---> physical:  2635 -- passed

// Addresses: 1000
// Page faults: 0
// Page fault rate: 0
// TLB hits: 55
// TLB hit rate: 0.055000

//                 ...done.