#include <stdio.h>
#include <inttypes.h>
#include <arpa/inet.h>
#include "trace_item.h"

#define TRACE_BUFSIZE 1024*1024

static FILE *trace_fd;
static int trace_buf_ptr;
static int trace_buf_end;
static struct trace_item *trace_buf;

//superscalar pipeline struct including the new stages
struct pipeline{
	//instruction buffer, holds 2 instructions
	//didn't make 2 separate items because IF_ID doesn't discriminate
	struct trace_item *IFID_first;
	struct trace_item *IFID_second;
	//pipeline items for alu/branch operations
	struct trace_item *REG_alu;
	struct trace_item *EX_alu;
	struct trace_item *MEM_alu;
	struct trace_item *WB_alu;
	//pipeline items for load/store items
	struct trace_item *REG_ls;
	struct trace_item *EX_ls;
	struct trace_item *MEM_ls;
	struct trace_item *WB_ls;
};

 //implementation of the branch prediction hash table
unsigned int pred_table[128];
void table_init()
{
	int i;
	for(i = 0; i < 128; i++) pred_table[(int)i] = NULL;
}

int check_pred(/*uint32_t addr, uint32_t pc, int tp*/)
{
	//immplement this whenever
	return 0;
}


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
	struct pipeline templine;
	struct trace_item no_op;
	struct trace_item squashed;
	no_op.type = ti_NOP;
	squashed.type = ti_SQUASHED;

	//create pipeline and fill with nops
	templine.IFID_first = &no_op;
	templine.IFID_second = &no_op;
	templine.REG_alu = &no_op;
	templine.REG_ls = &no_op;
	templine.EX_alu = &no_op;
	templine.EX_ls = &no_op;
	templine.MEM_alu = &no_op;
	templine.MEM_ls = &no_op;
	templine.WB_alu = &no_op;
	templine.WB_ls = &no_op;
	
	struct pipeline *tr_pipeline;
	tr_pipeline = &templine;
	table_init();
	
	//created a second tr_entry to read in a second instruction at a time
	struct trace_item *tr_entry_1;
	struct trace_item *tr_entry_2;
	size_t size;
	char *trace_file_name;
	int trace_view_on = 0;
	int trace_prediction_on = 0; //prediction type value
	
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
	if (argc == 4){ 
	  trace_view_on = atoi(argv[3]) ;
	  trace_prediction_on = atoi(argv[2]);
	}
	fprintf(stdout, "\n ** opening file %s\n", trace_file_name);

	trace_fd = fopen(trace_file_name, "rb");

	if (!trace_fd) {
	  fprintf(stdout, "\ntrace file %s not opened.\n\n", trace_file_name);
	  exit(0);
	}

	trace_init();
	
	int stall = 0;
	
	while(1){
	cycle_number++;
	if(stall == 0)
	{
		size = trace_get_item(&tr_entry_1);
		if(!size) tr_entry_1 = &no_op;
		size = trace_get_item(&tr_entry_2);
		if(!size) tr_entry_2 = &no_op;
	}
	else if(stall == 1)
	{
		tr_entry_1 = tr_entry_2;
		size = trace_get_item(&tr_entry_2);
		if(!size) tr_entry_2 = &no_op;
	}
	
	tr_pipeline->IFID_first = tr_entry_1;
	tr_pipeline->IFID_second = tr_entry_2;
	
	//PIPELINED SIMULATION: START
	//Printing the WB stage of the ALU/Branch pipeline
	   if (trace_view_on) {/* print the executed instruction if trace_view_on=1 */
       switch(tr_pipeline->WB_alu->type) {
         case ti_NOP:
           printf("[cycle %d] NOP:\n",cycle_number) ;
           break;
		 case ti_SQUASHED:
		   printf("[cycle %d] SQUASHED:\n", cycle_number);
		   break;
         case ti_RTYPE:
           printf("[cycle %d] RTYPE:",cycle_number) ;
           printf(" (PC: %x)(sReg_a: %d)(sReg_b: %d)(dReg: %d) \n", tr_pipeline->WB_alu->PC, tr_pipeline->WB_alu->sReg_a, tr_pipeline->WB_alu->sReg_b, tr_pipeline->WB_alu->dReg);
           break;
         case ti_ITYPE:
           printf("[cycle %d] ITYPE:",cycle_number) ;
           printf(" (PC: %x)(sReg_a: %d)(dReg: %d)(addr: %x)\n", tr_pipeline->WB_alu->PC, tr_pipeline->WB_alu->sReg_a, tr_pipeline->WB_alu->dReg, tr_pipeline->WB_alu->Addr);
           break;
         case ti_LOAD:
           printf("[cycle %d] LOAD:",cycle_number) ;      
           printf(" (PC: %x)(sReg_a: %d)(dReg: %d)(addr: %x)\n", tr_pipeline->WB_alu->PC, tr_pipeline->WB_alu->sReg_a, tr_pipeline->WB_alu->dReg, tr_pipeline->WB_alu->Addr);
           break;
         case ti_STORE:
           printf("[cycle %d] STORE:",cycle_number) ;      
           printf(" (PC: %x)(sReg_a: %d)(sReg_b: %d)(addr: %x)\n", tr_pipeline->WB_alu->PC, tr_pipeline->WB_alu->sReg_a, tr_pipeline->WB_alu->sReg_b, tr_pipeline->WB_alu->Addr);
           break;
         case ti_BRANCH:
           printf("[cycle %d] BRANCH:",cycle_number) ;
           printf(" (PC: %x)(sReg_a: %d)(sReg_b: %d)(addr: %x)\n", tr_pipeline->WB_alu->PC, tr_pipeline->WB_alu->sReg_a, tr_pipeline->WB_alu->sReg_b, tr_pipeline->WB_alu->Addr);
           break;
         case ti_JTYPE:
           printf("[cycle %d] JTYPE:",cycle_number) ;
           printf(" (PC: %x)(addr: %x)\n", tr_pipeline->WB_alu->PC,tr_pipeline->WB_alu->Addr);
           break;
         case ti_SPECIAL:
           printf("[cycle %d] SPECIAL:\n",cycle_number) ;      	
           break;
         case ti_JRTYPE:
           printf("[cycle %d] JRTYPE:",cycle_number) ;
           printf(" (PC: %x) (sReg_a: %d)(addr: %x)\n", tr_pipeline->WB_alu->PC, tr_pipeline->WB_alu->dReg, tr_pipeline->WB_alu->Addr);
           break;
       }
     }
	 
	   //printing the WB stage of the Load/Save pipeline
	   if (trace_view_on) {/* print the executed instruction if trace_view_on=1 */
       switch(tr_pipeline->WB_ls->type) {
         case ti_NOP:
           printf("[cycle %d] NOP:\n",cycle_number) ;
           break;
		 case ti_SQUASHED:
		   printf("[cycle %d] SQUASHED:\n", cycle_number);
		   break;
         case ti_RTYPE:
           printf("[cycle %d] RTYPE:",cycle_number) ;
           printf(" (PC: %x)(sReg_a: %d)(sReg_b: %d)(dReg: %d) \n", tr_pipeline->WB_ls->PC, tr_pipeline->WB_ls->sReg_a, tr_pipeline->WB_ls->sReg_b, tr_pipeline->WB_ls->dReg);
           break;
         case ti_ITYPE:
           printf("[cycle %d] ITYPE:",cycle_number) ;
           printf(" (PC: %x)(sReg_a: %d)(dReg: %d)(addr: %x)\n", tr_pipeline->WB_ls->PC, tr_pipeline->WB_ls->sReg_a, tr_pipeline->WB_ls->dReg, tr_pipeline->WB_ls->Addr);
           break;
         case ti_LOAD:
           printf("[cycle %d] LOAD:",cycle_number) ;      
           printf(" (PC: %x)(sReg_a: %d)(dReg: %d)(addr: %x)\n", tr_pipeline->WB_ls->PC, tr_pipeline->WB_ls->sReg_a, tr_pipeline->WB_ls->dReg, tr_pipeline->WB_ls->Addr);
           break;
         case ti_STORE:
           printf("[cycle %d] STORE:",cycle_number) ;      
           printf(" (PC: %x)(sReg_a: %d)(sReg_b: %d)(addr: %x)\n", tr_pipeline->WB_ls->PC, tr_pipeline->WB_ls->sReg_a, tr_pipeline->WB_ls->sReg_b, tr_pipeline->WB_ls->Addr);
           break;
         case ti_BRANCH:
           printf("[cycle %d] BRANCH:",cycle_number) ;
           printf(" (PC: %x)(sReg_a: %d)(sReg_b: %d)(addr: %x)\n", tr_pipeline->WB_ls->PC, tr_pipeline->WB_ls->sReg_a, tr_pipeline->WB_ls->sReg_b, tr_pipeline->WB_ls->Addr);
           break;
         case ti_JTYPE:
           printf("[cycle %d] JTYPE:",cycle_number) ;
           printf(" (PC: %x)(addr: %x)\n", tr_pipeline->WB_ls->PC,tr_pipeline->WB_ls->Addr);
           break;
         case ti_SPECIAL:
           printf("[cycle %d] SPECIAL:\n",cycle_number) ;      	
           break;
         case ti_JRTYPE:
           printf("[cycle %d] JRTYPE:",cycle_number) ;
           printf(" (PC: %x) (sReg_a: %d)(addr: %x)\n", tr_pipeline->WB_ls->PC, tr_pipeline->WB_ls->dReg, tr_pipeline->WB_ls->Addr);
           break;
       }
     }
	
	//EX->MEM->WB push happens no matter what, pushes all of them along
	tr_pipeline->WB_alu = tr_pipeline->MEM_alu;
	tr_pipeline->WB_ls = tr_pipeline->MEM_ls;
	tr_pipeline->MEM_alu = tr_pipeline->EX_alu;
	tr_pipeline->MEM_ls = tr_pipeline->EX_ls;
	tr_pipeline->EX_alu = tr_pipeline->REG_alu;
	tr_pipeline->EX_ls = tr_pipeline->REG_ls;
	
	struct trace_item *first;
	first = tr_pipeline->IFID_first;
	struct trace_item *second;
	second = tr_pipeline->IFID_second;
	struct trace_item *prev;
	prev = tr_pipeline->REG_ls;
	
	//switch statement to handle movement from IF+ID to REG
	switch(first->type){
		//cases on load OR store
		case ti_LOAD:
		case ti_STORE:
			switch(second->type){
				//first is L/S and second is ALU
				case ti_RTYPE:
				case ti_ITYPE:
				case ti_BRANCH:
				case ti_JTYPE:
				case ti_JRTYPE:
				case ti_SPECIAL:
					//previous load/store is a load, possible load-use hazard on previous
					if (prev->type == ti_LOAD)
					{
						//no load-use on first
						if(prev->dReg != first->sReg_a && prev->dReg != first->sReg_b)
						{
							//no load-use on second
							if(prev->dReg != second->sReg_a && prev->dReg != second->sReg_b)
							{
								//no load-use between first and second
								if(first->type == ti_LOAD && first->dReg != second->sReg_a && first->dReg != second->sReg_b)
								{
									tr_pipeline->REG_ls = tr_pipeline->IFID_first;
									tr_pipeline->REG_alu = tr_pipeline->IFID_second;
									stall = 0; //both issued
								}
								//if first isn't a load then load-use doesn't matter, issue both
								else if(first->type != ti_LOAD)
								{
									tr_pipeline->REG_ls = tr_pipeline->IFID_first;
									tr_pipeline->REG_alu = tr_pipeline->IFID_second;
									stall = 0; //both issued
								}
							}
							//load-use on second, issue first
							else
							{
								tr_pipeline->REG_ls = tr_pipeline->IFID_first;
								tr_pipeline->REG_alu = &no_op;
								stall = 1; //first issued
							}
						}
						//load-use on first, don't issue either
						else
						{
							tr_pipeline->REG_ls = &no_op;
							tr_pipeline->REG_alu = &no_op;
							stall = 2; //none issued
						}
					}
					//previous load/store isn't a load, no load-use hazard on previous
					else
					{
						//no load-use between the two, issue both
						if(first->type == ti_LOAD && first->dReg != second->sReg_a && first->dReg != second->sReg_b)
						{
							tr_pipeline->REG_ls = tr_pipeline->IFID_first;
							tr_pipeline->REG_alu = tr_pipeline->IFID_second;
							stall = 0; //both issued
						}
						else if(first->type != ti_LOAD)
						{
							tr_pipeline->REG_ls = tr_pipeline->IFID_first;
							tr_pipeline->REG_alu = tr_pipeline->IFID_second;
							stall = 0; //both issued
						}
						//load-use between the two, issue first
						else
						{
							tr_pipeline->REG_ls = tr_pipeline->IFID_first;
							tr_pipeline->REG_alu = &no_op;
							stall = 1; //first issued
						}
					}
				break;
				
				
				//first is L/S and second is also L/S
				case ti_LOAD:
				case ti_STORE:
					if (prev->type == ti_LOAD)
					{
						if(prev->dReg != first->sReg_a && prev->dReg != first->sReg_b)
						{
							tr_pipeline->REG_ls = tr_pipeline->IFID_first;
							tr_pipeline->REG_alu = &no_op;
							stall = 1;
						}
						else
						{
							tr_pipeline->REG_ls = &no_op;
							tr_pipeline->REG_alu = &no_op;
							stall = 2; //none issued
						}
					}
					else
					{
						tr_pipeline->REG_ls = tr_pipeline->IFID_first;
						tr_pipeline->REG_alu = &no_op;
						stall = 1; //first issued
					}
				break;
			}
		break;
		
		//cases of all ALU operations except if the first is a branch
		case ti_RTYPE:
		case ti_ITYPE:
		case ti_JTYPE:
		case ti_JRTYPE:
		case ti_SPECIAL:
			switch(second->type){
				//first is ALU (not including branch) and second is L/S
				case ti_LOAD:
				case ti_STORE:
					 if (prev->type == ti_LOAD)
					 {
						 if(prev->dReg != first->sReg_a && prev->dReg != first->sReg_b)
						 {
							if(prev->dReg != second->sReg_a && prev->dReg != second->sReg_b)
							{
								if(second->type = ti_LOAD && second->dReg != first->sReg_a && second->dReg != first->sReg_b)
								{
									tr_pipeline->REG_alu = tr_pipeline->IFID_first;
									tr_pipeline->REG_ls = tr_pipeline->IFID_second;
									stall = 0;
								}
								else if(second->type != ti_LOAD)
								{
									tr_pipeline->REG_alu = tr_pipeline->IFID_first;
									tr_pipeline->REG_ls = tr_pipeline->IFID_second;
									stall = 0;
								}
								else
								{
									tr_pipeline->REG_alu = tr_pipeline->IFID_first;
									tr_pipeline->REG_ls = &no_op;
									stall = 1;
								}
							}
							else
							{
								tr_pipeline->REG_alu = tr_pipeline->IFID_first;
								tr_pipeline->REG_ls = &no_op;
								stall = 1;
							}
						 }
						 else
						 {
							 tr_pipeline->REG_alu = &no_op;
							 tr_pipeline->REG_ls = &no_op;
							 stall = 2;
						 }
					 }
					 else
					 {
						if(second->type = ti_LOAD && second->dReg != first->sReg_a && second->dReg != first->sReg_b)
						{
							tr_pipeline->REG_alu = tr_pipeline->IFID_first;
							tr_pipeline->REG_ls = tr_pipeline->IFID_second;
							stall = 0;
						}
						else if(second->type != ti_LOAD)
						{
							tr_pipeline->REG_alu = tr_pipeline->IFID_first;
							tr_pipeline->REG_ls = tr_pipeline->IFID_second;
							stall = 0;
						}
						else
						{
							tr_pipeline->REG_alu = tr_pipeline->IFID_first;
							tr_pipeline->REG_ls = &no_op;
							stall = 1;
						}
					 }
				break;
				
				//first is ALU(not including branch) and second is ALU
				case ti_RTYPE:
				case ti_ITYPE:
				case ti_BRANCH:
				case ti_JTYPE:
				case ti_JRTYPE:
				case ti_SPECIAL:
					if (prev->type == ti_LOAD)
					{
						if(prev->dReg != first->sReg_a && prev->dReg != first->sReg_b)
						{
							tr_pipeline->REG_alu = tr_pipeline->IFID_first;
							tr_pipeline->REG_ls = &no_op;
							stall = 1; //issue first
						}
						else
						{
							tr_pipeline->REG_alu = &no_op;
							tr_pipeline->REG_ls = &no_op;
							stall = 2; //issue none
						}
					}
					else
					{
						tr_pipeline->REG_alu = tr_pipeline->IFID_first;
						tr_pipeline->REG_ls = &no_op;
						stall = 1; //issue first
					}
				break;
			}
		break;
		
		//case that first is a branch
		case ti_BRANCH:
			if(prev->type == ti_LOAD)
			{
				if(prev->dReg != first->sReg_a && prev->dReg != first->sReg_b)
				{
					tr_pipeline->REG_alu = tr_pipeline->IFID_first;
					tr_pipeline->REG_ls = &no_op;
					stall = 1; //issue first
				}
				else
				{
					tr_pipeline->REG_alu = &no_op;
					tr_pipeline->REG_ls = &no_op;
					stall = 2; //issue none
				}
			}
			else
			{
				tr_pipeline->REG_alu = tr_pipeline->IFID_first;
				tr_pipeline->REG_ls = &no_op;
				stall = 1; //issue first
			}
		break;
		case ti_NOP:
			tr_pipeline->REG_alu = tr_pipeline->IFID_second;
			tr_pipeline->REG_ls = &no_op;
			stall = 1;
		break;
	}
	
	
	//handling of branch prediction
	
	
	
	if( tr_pipeline->IFID_first->type == ti_NOP && tr_pipeline->IFID_second->type == ti_NOP && tr_pipeline->REG_alu->type == ti_NOP &&  tr_pipeline->EX_alu->type == ti_NOP  &&  tr_pipeline->MEM_alu->type == ti_NOP && tr_pipeline->WB_alu->type == ti_NOP && tr_pipeline->REG_ls->type == ti_NOP && tr_pipeline->EX_ls->type == ti_NOP &&  tr_pipeline->MEM_ls->type == ti_NOP  &&  tr_pipeline->WB_ls->type == ti_NOP )
	{
		printf("+ Simulation terminates at cycle : %u\n", cycle_number);
      	break;
	}
	}
	trace_uninit();
	
	exit(0);
}
