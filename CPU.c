/**************************************************************/
/* CS/COE 1541				 			
   just compile with gcc -o pipeline pipeline.c			
   and execute using							
   ./pipeline  /afs/cs.pitt.edu/courses/1541/short_traces/sample.tr	0  
***************************************************************/

#include <stdio.h>
#include <inttypes.h>
#include <arpa/inet.h>
#include "trace_item.h" 

#define TRACE_BUFSIZE 1024*1024

static FILE *trace_fd;
static int trace_buf_ptr;
static int trace_buf_end;
static struct trace_item *trace_buf;

int is_big_endian(void)
{
    union {
        uint32_t i;
        char c[4];
    } bint = {0x01020304};

    return bint.c[0] == 1; 
}

uint32_t my_ntohl(uint32_t x)
{
  u_char *s = (u_char *)&x;
  return (uint32_t)(s[3] << 24 | s[2] << 16 | s[1] << 8 | s[0]);
}

void trace_init()
{
  trace_buf = malloc(sizeof(struct trace_item) * TRACE_BUFSIZE);

  if (!trace_buf) {
    fprintf(stdout, "** trace_buf not allocated\n");
    exit(-1);
  }

  trace_buf_ptr = 0;
  trace_buf_end = 0;
}

void trace_uninit()
{
  free(trace_buf);
  fclose(trace_fd);
}

int trace_get_item(struct trace_item **item)
{
  int n_items;

  if (trace_buf_ptr == trace_buf_end) {	/* if no more unprocessed items in the trace buffer, get new data  */
    n_items = fread(trace_buf, sizeof(struct trace_item), TRACE_BUFSIZE, trace_fd);
    if (!n_items) return 0;				/* if no more items in the file, we are done */

    trace_buf_ptr = 0;
    trace_buf_end = n_items;			/* n_items were read and placed in trace buffer */
  }

  *item = &trace_buf[trace_buf_ptr];	/* read a new trace item for processing */
  trace_buf_ptr++;
  
  if (is_big_endian()) {
    (*item)->PC = my_ntohl((*item)->PC);
    (*item)->Addr = my_ntohl((*item)->Addr);
  }

  return 1;
}

int main(int argc, char **argv)
{
  
  //creating a "pipeline" struct to store the entries that are in each stage of the pipeline
  struct pipeline {
	  struct trace_item *IF;
	  struct trace_item *ID;
	  struct trace_item *EX;
	  struct trace_item *MEM;
	  struct trace_item *WB;
  };  //typo correction
  struct pipeline templine;
  struct trace_item no_op;
  no_op.type = ti_NOP;
  templine.IF = &no_op;
  templine.ID = &no_op;
  templine.EX = &no_op;
  templine.MEM = &no_op;
  templine.WB = &no_op;
  
  struct pipeline *tr_pipeline;
  tr_pipeline = &templine;
  
  //fill pipeline with nops to start
  

  
  /*tr_pipeline->IF->type = ti_NOP;
  tr_pipeline->ID->type = ti_NOP;
  tr_pipeline->EX->type = ti_NOP;
  tr_pipeline->MEM->type = ti_NOP;
  tr_pipeline->WB->type = ti_NOP;*/
  
  struct trace_item *tr_entry;
  size_t size;
  char *trace_file_name;
  int trace_view_on = 0;
  
  unsigned char t_type = 0;
  unsigned char t_sReg_a= 0;
  unsigned char t_sReg_b= 0;
  unsigned char t_dReg= 0;
  unsigned int t_PC = 0;
  unsigned int t_Addr = 0;

  unsigned int cycle_number = 0;

  if (argc < 3) {
    fprintf(stdout, "\nUSAGE: tv <trace_file> <prediction method> <switch - any character>\n");
    fprintf(stdout, "\n(switch) to turn on or off individual item view.\n\n");
    exit(0);
  }
    
  trace_file_name = argv[1];
  if (argc == 4) trace_view_on = atoi(argv[3]) ;

  fprintf(stdout, "\n ** opening file %s\n", trace_file_name);

  trace_fd = fopen(trace_file_name, "rb");

  if (!trace_fd) {
    fprintf(stdout, "\ntrace file %s not opened.\n\n", trace_file_name);
    exit(0);
  }

  trace_init();
  
  int stall = 0;  //if stall is 0, get new instruction; if not, continue with previous instruction
  		//NOT CURRENTLY IN USE

  while(1) {
    cycle_number++;
    if(stall == 0)
    {
      size = trace_get_item(&tr_entry);
    }
   
    if (!size) {       /* no more instructions (trace_items) to simulate */
      tr_entry = &no_op;
    }
    else{              /* parse the next instruction to simulate */
      t_type = tr_entry->type;
      t_sReg_a = tr_entry->sReg_a;
      t_sReg_b = tr_entry->sReg_b;
      t_dReg = tr_entry->dReg;
      t_PC = tr_entry->PC;
      t_Addr = tr_entry->Addr;
    }  

	//PIPELINED SIMULATION: START
	//switch/case the same as single cycle, statements will vary due to pipelining
	
	//Print what's in the WB stage
    if (trace_view_on) {/* print the executed instruction if trace_view_on=1 */
       switch(tr_pipeline->WB->type) {
         case ti_NOP:
           printf("[cycle %d] NOP:\n",cycle_number) ;
           break;
         case ti_RTYPE:
           printf("[cycle %d] RTYPE:",cycle_number) ;
           printf(" (PC: %x)(sReg_a: %d)(sReg_b: %d)(dReg: %d) \n", tr_pipeline->WB->PC, tr_pipeline->WB->sReg_a, tr_pipeline->WB->sReg_b, tr_pipeline->WB->dReg);
           break;
         case ti_ITYPE:
           printf("[cycle %d] ITYPE:",cycle_number) ;
           printf(" (PC: %x)(sReg_a: %d)(dReg: %d)(addr: %x)\n", tr_pipeline->WB->PC, tr_pipeline->WB->sReg_a, tr_pipeline->WB->dReg, tr_pipeline->WB->Addr);
           break;
         case ti_LOAD:
           printf("[cycle %d] LOAD:",cycle_number) ;      
           printf(" (PC: %x)(sReg_a: %d)(dReg: %d)(addr: %x)\n", tr_pipeline->WB->PC, tr_pipeline->WB->sReg_a, tr_pipeline->WB->dReg, tr_pipeline->WB->Addr);
           break;
         case ti_STORE:
           printf("[cycle %d] STORE:",cycle_number) ;      
           printf(" (PC: %x)(sReg_a: %d)(sReg_b: %d)(addr: %x)\n", tr_pipeline->WB->PC, tr_pipeline->WB->sReg_a, tr_pipeline->WB->sReg_b, tr_pipeline->WB->Addr);
           break;
         case ti_BRANCH:
           printf("[cycle %d] BRANCH:",cycle_number) ;
           printf(" (PC: %x)(sReg_a: %d)(sReg_b: %d)(addr: %x)\n", tr_pipeline->WB->PC, tr_pipeline->WB->sReg_a, tr_pipeline->WB->sReg_b, tr_pipeline->WB->Addr);
           break;
         case ti_JTYPE:
           printf("[cycle %d] JTYPE:",cycle_number) ;
           printf(" (PC: %x)(addr: %x)\n", tr_pipeline->WB->PC,tr_pipeline->WB->Addr);
           break;
         case ti_SPECIAL:
           printf("[cycle %d] SPECIAL:",cycle_number) ;      	
           break;
         case ti_JRTYPE:
           printf("[cycle %d] JRTYPE:",cycle_number) ;
           printf(" (PC: %x) (sReg_a: %d)(addr: %x)\n", tr_pipeline->WB->PC, tr_pipeline->WB->dReg, tr_pipeline->WB->Addr);
           break;
       }
     }

     
     //push everything else along
     
     tr_pipeline->WB = tr_pipeline->MEM;
     tr_pipeline->MEM = tr_pipeline->EX;
     tr_pipeline->EX = tr_pipeline->ID;
     tr_pipeline->ID = tr_pipeline->IF;

	
	switch(tr_entry -> type) {   
		/*case ti_RTYPE:
		break;
		
		case ti_ITYPE:
		break;
		
		case ti_LOAD:
		break;
		
		case ti_STORE:
		break;
		
		case ti_BRANCH:
		break;
		
		case ti_JTYPE:
		break;
		
		case ti_SPECIAL:
		break;
		
		case ti_JRTYPE:
		break;			*/
		
		default:
		tr_pipeline->IF = tr_entry;
		break;
	}
	
	if( tr_pipeline->IF->type == ti_NOP && tr_pipeline->ID->type == ti_NOP && tr_pipeline->EX->type == ti_NOP &&  tr_pipeline->MEM->type == ti_NOP  &&  tr_pipeline->WB->type == ti_NOP  )
	{
		printf("+ Simulation terminates at cycle : %u\n", cycle_number);
      		break;
	}
	
	
// SIMULATION OF A SINGLE CYCLE cpu IS TRIVIAL - EACH INSTRUCTION IS EXECUTED
// IN ONE CYCLE

    // if (trace_view_on) {/* print the executed instruction if trace_view_on=1 */
      // switch(tr_entry->type) {
        // case ti_NOP:
          // printf("[cycle %d] NOP:",cycle_number) ;
          // break;
        // case ti_RTYPE:
          // printf("[cycle %d] RTYPE:",cycle_number) ;
          // printf(" (PC: %x)(sReg_a: %d)(sReg_b: %d)(dReg: %d) \n", tr_entry->PC, tr_entry->sReg_a, tr_entry->sReg_b, tr_entry->dReg);
          // break;
        // case ti_ITYPE:
          // printf("[cycle %d] ITYPE:",cycle_number) ;
          // printf(" (PC: %x)(sReg_a: %d)(dReg: %d)(addr: %x)\n", tr_entry->PC, tr_entry->sReg_a, tr_entry->dReg, tr_entry->Addr);
          // break;
        // case ti_LOAD:
          // printf("[cycle %d] LOAD:",cycle_number) ;      
          // printf(" (PC: %x)(sReg_a: %d)(dReg: %d)(addr: %x)\n", tr_entry->PC, tr_entry->sReg_a, tr_entry->dReg, tr_entry->Addr);
          // break;
        // case ti_STORE:
          // printf("[cycle %d] STORE:",cycle_number) ;      
          // printf(" (PC: %x)(sReg_a: %d)(sReg_b: %d)(addr: %x)\n", tr_entry->PC, tr_entry->sReg_a, tr_entry->sReg_b, tr_entry->Addr);
          // break;
        // case ti_BRANCH:
          // printf("[cycle %d] BRANCH:",cycle_number) ;
          // printf(" (PC: %x)(sReg_a: %d)(sReg_b: %d)(addr: %x)\n", tr_entry->PC, tr_entry->sReg_a, tr_entry->sReg_b, tr_entry->Addr);
          // break;
        // case ti_JTYPE:
          // printf("[cycle %d] JTYPE:",cycle_number) ;
          // printf(" (PC: %x)(addr: %x)\n", tr_entry->PC,tr_entry->Addr);
          // break;
        // case ti_SPECIAL:
          // printf("[cycle %d] SPECIAL:",cycle_number) ;      	
          // break;
        // case ti_JRTYPE:
          // printf("[cycle %d] JRTYPE:",cycle_number) ;
          // printf(" (PC: %x) (sReg_a: %d)(addr: %x)\n", tr_entry->PC, tr_entry->dReg, tr_entry->Addr);
          // break;
      // }
    // }
  }

  trace_uninit();

  exit(0);
}

