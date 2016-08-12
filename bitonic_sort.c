/* 
 *   India Buckley
 *   File:
 *   p4.c
 *
 * Purpose:
 *   Implement serial bitonic sort of a list that is either user
 *   input or generated by the program using a random number
 *   generator.
 *
 * Compile:  
 *   mpicc -g -Wall -o p4 p4.c
 *
 * Run:
 *   ./p4 <n> <'i'|'g'>
 *      n = number of elements in the list (a power of 2)
 *      'i':  user will enter list (no quotes)
 *      'g':  program should generate list (no quotes)
 *
 * Input:
 *    If command line option 'i' is used, an n-element list of
 *    ints
 *
 * Output:
 *    The unsorted list and the sorted list.
 *
 * Notes:
 *    1.  If the list is randomly generated, the keys are in the range 
 *       1 -- KEY_MAX.
 *    2.  The size of the list, n, should be a power of 2:  the program
 *       doesn't check that this is the case.
 *
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <mpi.h>

/* Successive subsequences will switch between
 * increasing and decreasing bitonic splits.
 */
#define INCR 0
#define DECR 1
#define KEY_MAX 100

void Get_args(int argc, char* argv[], int* n_p, char* ig_p);
void Read_list(char title[], int A[], int n, int my_rank);
void Gen_list(int global_A[], int n, int my_rank);
int Compare (const void* a_p, const void*b_p);
void Bitonic_sort(int local_A[], int local_partner[], int local_merge[], int local_n, int n, int my_rank, int p, MPI_Comm comm);
void Bitonic_sort_incr(int local_A[], int local_partner[], int local_merge[], int local_n, int max_stage, int n, int my_rank, MPI_Comm comm);
void Bitonic_sort_decr(int local_A[], int local_partner[], int local_merge[], int local_n, int max_stage, int n, int my_rank, MPI_Comm comm);
void Merge_split_lo(int local_list[], int local_rec[], int local_merge[], int local_n, MPI_Comm comm);
void Merge_split_hi(int local_list[], int local_rec[], int local_merge[], int local_n, MPI_Comm comm);
void Print_list(char* title, int A[], int n);
int  Reverse(int order);
void Swap(int *a_p, int *b_p);

/*-------------------------------------------------------------------*/
int main(int argc, char* argv[]) {
   int n, local_n, *global_A, *local_A, *local_partner, *local_merge, p, my_rank;
   MPI_Comm comm;
   char   ig;

   MPI_Init(&argc, &argv);
   comm = MPI_COMM_WORLD;
   MPI_Comm_size(comm, &p);
   MPI_Comm_rank(comm, &my_rank);

   Get_args(argc, argv, &n, &ig);
   local_n = n/p;
   local_partner = malloc(local_n*sizeof(int));
   local_merge = malloc(local_n*sizeof(int));
   local_A = malloc(local_n*sizeof(int));
   global_A = malloc(n*sizeof(int));
   

   if (ig == 'i')
      Read_list("Enter the list", global_A, n, my_rank);
   else
      Gen_list(global_A, n, my_rank);
   MPI_Scatter(global_A, local_n, MPI_INT, local_A, local_n, MPI_INT, 0, comm);

   if(my_rank==0){
      Print_list("The unsorted list is", global_A, n);
   }

   Bitonic_sort(local_A, local_partner, local_merge, local_n, n, my_rank, p, comm);
   MPI_Gather(local_A, local_n, MPI_INT, global_A, local_n, MPI_INT, 0, comm);

   if(my_rank==0){
      Print_list("The sorted list is", global_A, n);
   }

   free(global_A);
   free(local_A);
   free(local_partner);
   free(local_merge);
   MPI_Finalize();
   return 0;
}  /* main */

/*---------------------------------------------------------------------
 * Function:  Get_args
 * Purpose:   Get the command line arguments
 * In args:   argc, argv
 * Out args:  n_p:  pointer to list size
 *         ig_p:  pointer to 'i' if user will input list,
 *            otherwise pointer to '
 */
void Get_args(int argc, char* argv[], int* n_p, char* ig_p) {
   if (argc != 3) {
      fprintf(stderr, "usage: %s <n> <'i'|'g'>\n", argv[0]);
      fprintf(stderr, "   n = number of elements in the list (a power of 2)\n");
      fprintf(stderr, "   'i':  user will enter list (no quotes)\n");
      fprintf(stderr, "   'g':  program should generate list (no quotes)\n");
      exit(0);
   }
   *n_p = strtol(argv[1], NULL, 10);
   *ig_p = argv[2][0];
}  /* Get_args */

/*---------------------------------------------------------------------
 * Function:  Read_list
 * Purpose:   Read in a list of ints
 * In arg:    n, my_rank
 * Out arg:   A
 */
void Read_list(char title[], int A[], int n, int my_rank) {
   int i;
   if(my_rank==0){
      for (i = 0; i < n; i++)
      scanf("%d", &A[i]);
   }

}  /* Read_list */


/*---------------------------------------------------------------------
 * Function:  Reverse
 * Purpose:   Switch INCR to DECR and DECR to INCR
 * In arg:    order
 * Ret val:   INCR if order is DECR, DECR if order is INCR
 */
int  Reverse(int order) {
   if (order == INCR)
      return DECR;
   else
      return INCR;
} /* Reverse */


/*---------------------------------------------------------------------
 * Function:    Swap
 * Purpose:     Swap the values referred to by a_p and b_p
 * In/out args:  a_p, b_p
 */
void Swap(int *a_p, int *b_p) {
   int temp = *a_p;
   *a_p = *b_p;
   *b_p = temp;
}  /* Swap */


/*---------------------------------------------------------------------
 * Function:   Gen_list
 * Purpose:    Use a random number generator to generate an n-element
 *             list
 * In arg:     n
 * Out arg:    A
 * Note:       Elements of the list are in the range 1 -- KEY_MAX
 */
void Gen_list(int global_A[], int n, int my_rank) {
   int i;
   if(my_rank==0){
      srandom(1);
      for (i = 0; i < n; i++)
         global_A[i] = 1 + random() % KEY_MAX;
   }
    
}  /* Gen_list */ 

/*-------------------------------------------------------------------
 * Function:        Bitonic_sort
 * Purpose:         Implement bitonic sort of a list of ints
 */

 int Compare (const void* a_p, const void*b_p){
   int a = *((int*)a_p);
   int b = *((int*)b_p);
   if(a<b)
      return -1;
   else if(a==b)
      return 0;
   else
      return 1;
 } /*Compare*/
/*-------------------------------------------------------------------
 * Function:        Bitonic_sort
 * Purpose:         Implement bitonic sort of a list of ints
 */
void Bitonic_sort(int local_A[], int local_partner[], int local_merge[], int local_n, int n, int my_rank, int p, MPI_Comm comm) {
   unsigned p_count, and_bit;
   int max_stage;


   qsort(local_A, local_n, sizeof(int), Compare);  
#  ifdef DEBUG
      printf("Proc: %d in stage #%d with %u processes involved",my_rank, max_stage, p_count)
#  endif
   for (p_count = 2, and_bit = 2, max_stage = 1; p_count <= p; p_count <<= 1, and_bit <<= 1, max_stage++) {
      if ((my_rank & and_bit) == 0)
         Bitonic_sort_incr(local_A, local_partner, local_merge, local_n, max_stage, n, my_rank, comm);
      else
         Bitonic_sort_decr(local_A, local_partner, local_merge, local_n, max_stage, n, my_rank, comm);
   }
}  /* Bitonic_sort */

/*-------------------------------------------------------------------
 * Function:      Bitonic_sort_incr
 * Purpose:       Use parallel bitonic sort to sort a list into
 *                increasing order.  This implements a butterfly
 *                communication scheme among the processes
 */
void Bitonic_sort_incr(int local_A[], int local_partner[], int local_merge[], int local_n, int max_stage, int n, int my_rank, MPI_Comm comm) {
   int stage;
   int partner;
   int* tmp = NULL;
   unsigned eor_bit = 1 << (max_stage - 1);

   if(my_rank==0){
      tmp = malloc(n*sizeof(int));
   }

   for (stage = 0; stage < max_stage; stage++) {
      partner = my_rank ^ eor_bit;
      MPI_Sendrecv(local_A, local_n, MPI_INT, partner, 0, local_partner, local_n, MPI_INT, partner, 0, comm, MPI_STATUS_IGNORE);
      if (my_rank < partner)
         Merge_split_lo(local_A, local_partner, local_merge, local_n, comm);
      else
         Merge_split_hi(local_A, local_partner, local_merge, local_n, comm);
      eor_bit >>= 1;
     
   } 
   free(tmp);   
}  /* Bitonic_sort_incr */


/*-------------------------------------------------------------------
 * Function:      Bitonic_sort_decr
 * Purpose:       Use parallel bitonic sort to sort a list into
 *                decreasing order.  This implements a butterfly
 *                communication scheme among the processes
 */
void Bitonic_sort_decr(int local_A[], int local_partner[], int local_merge[], int local_n, int max_stage, int n, int my_rank, MPI_Comm comm) {
   int stage;
   int partner;
   int* tmp = NULL;
   unsigned eor_bit = 1 << (max_stage - 1);

   if(my_rank==0){
      tmp = malloc(n*sizeof(int));
   }

   for (stage = 0; stage < max_stage; stage++) {
      partner = my_rank ^ eor_bit;
      MPI_Sendrecv(local_A, local_n, MPI_INT, partner, 0, local_partner, local_n, MPI_INT, partner, 0, comm, MPI_STATUS_IGNORE);       
      if (my_rank > partner)
         Merge_split_lo(local_A, local_partner, local_merge, local_n, comm);
      else
         Merge_split_hi(local_A, local_partner, local_merge, local_n, comm);
      eor_bit >>= 1;
   } 
   free(tmp);
       
}  /* Bitonic_sort_decr */


/*---------------------------------------------------------------------
 * Function:   Print_list
 * Purpose:    Print the elements of A
 * In args:    all
 */
void Print_list(char* title, int A[], int n) {
   int i;

   printf("%s\n", title);
   for (i = 0; i < n; i++)
      printf("%d ", A[i]);
   printf("\n");
}  /* Print_list */

/*-------------------------------------------------------------------
 * Function:        Merge_split_lo
 * Purpose:         Merge two sublists that have smaller numbers 
 */
void Merge_split_lo(int local_A[], int local_partner[], int local_merge[], int local_n, MPI_Comm comm) {
   int k = 0; 
   int j = 0;
   int i = 0;
 
   for (k = 0; k < local_n; k++){
      if (local_A[i] <= local_partner[j]) {
         local_merge[k] = local_A[i];
         i++;
      } else {
         local_merge[k] = local_partner[j];
         j++;
      }
   }
   memcpy(local_A, local_merge, local_n*sizeof(int));
}  /* Merge_split_lo */


/*-------------------------------------------------------------------
 * Function:        Merge_split_hi
 * Purpose:         Merge two sublists that have larger numbers

 */
void Merge_split_hi(int local_A[], int local_partner[], int local_merge[], int local_n, MPI_Comm comm) {
   int i = local_n-1;
   int j = local_n-1;
   int k;

   for (k = local_n-1; k > 0; k--){
      if (local_A[i] >= local_partner[j]){
         local_merge[k] = local_A[j];
         i--;
      }else{
         local_merge[k] = local_partner[j];
         j--;
      }
   }
   memcpy(local_A, local_merge, local_n*sizeof(int));

}  /* Merge_split_hi */
