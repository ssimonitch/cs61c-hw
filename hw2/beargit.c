#include <stdio.h>
#include <string.h>
#include <math.h>

#include <unistd.h>
#include <sys/stat.h>

#include "beargit.h"
#include "util.h"

/* Implementation Notes:
 *
 * - Functions return 0 if successful, 1 if there is an error.
 * - All error conditions in the function description need to be implemented
 *   and written to stderr. We catch some additional errors for you in main.c.
 * - Output to stdout needs to be exactly as specified in the function description.
 * - Only edit this file (beargit.c)
 * - You are given the following helper functions:
 *   * fs_mkdir(dirname): create directory <dirname>
 *   * fs_rm(filename): delete file <filename>
 *   * fs_mv(src,dst): move file <src> to <dst>, overwriting <dst> if it exists
 *   * fs_cp(src,dst): copy file <src> to <dst>, overwriting <dst> if it exists
 *   * write_string_to_file(filename,str): write <str> to filename (overwriting contents)
 *   * read_string_from_file(filename,str,size): read a string of at most <size> (incl.
 *     NULL character) from file <filename> and store it into <str>. Note that <str>
 *     needs to be large enough to hold that string.
 *  - You NEED to test your code. The autograder we provide does not contain the
 *    full set of tests that we will run on your code. See "Step 5" in the homework spec.
 */

/* beargit init
 *
 * - Create .beargit directory
 * - Create empty .beargit/.index file
 * - Create .beargit/.prev file containing 0..0 commit id
 *
 * Output (to stdout):
 * - None if successful
 */

int beargit_init(void) {
  fs_mkdir(".beargit");

  FILE* findex = fopen(".beargit/.index", "w");
  fclose(findex);
  
  write_string_to_file(".beargit/.prev", "0000000000000000000000000000000000000000");

  return 0;
}


/* beargit add <filename>
 * 
 * - Append filename to list in .beargit/.index if it isn't in there yet
 *
 * Possible errors (to stderr):
 * >> ERROR: File <filename> already added
 *
 * Output (to stdout):
 * - None if successful
 */

int beargit_add(const char* filename) {
  FILE* findex = fopen(".beargit/.index", "r");
  FILE *fnewindex = fopen(".beargit/.newindex", "w");

  char line[FILENAME_SIZE];
  while(fgets(line, sizeof(line), findex)) {
    strtok(line, "\n");
    if (strcmp(line, filename) == 0) {
      fprintf(stderr, "ERROR: File %s already added\n", filename);
      fclose(findex);
      fclose(fnewindex);
      fs_rm(".beargit/.newindex");
      return 3;
    }

    fprintf(fnewindex, "%s\n", line);
  }

  fprintf(fnewindex, "%s\n", filename);
  fclose(findex);
  fclose(fnewindex);

  fs_mv(".beargit/.newindex", ".beargit/.index");

  return 0;
}


/* beargit rm <filename> */
int beargit_rm(const char* filename) {
  FILE *fp     = fopen(".beargit/.index", "r");
  FILE *fp_new = fopen(".beargit/.newindex", "w");

  char line[FILENAME_SIZE];
  int found = 0;

  while(fgets(line, sizeof(line), fp)) {
    strtok(line, "\n");
    if (strcmp(line, filename) == 0)
      found = 1;
    else
      fprintf(fp_new, "%s\n", line);
  }

  if (found == 0) {
    fprintf(stderr, "ERROR: File %s not tracked\n", filename);
    fclose(fp);
    fclose(fp_new);
    fs_rm(".beargit/.newindex");
    return 1;
  }

  fclose(fp);
  fclose(fp_new);

  fs_mv(".beargit/.newindex", ".beargit/.index");

  return 0;
}

/* beargit commit -m <msg>
 *
 * stderr:
 * If the commit message does not contain the exact string "GO BEARS!", 
 * then you must output the following to stderr and return 1:
 *    beargit commit -m "G-O- -B-E-A-R-S-!"
 *    ERROR: Message must contain "GO BEARS!"
 * 
 * stdout:
 * If the commit message does contain the string "GO BEARS!", 
 * then the function should produce no output and return 0.
 */

const char* go_bears = "GO BEARS!";

/*
 * First, check whether the commit string contains "GO BEARS!". If not, display an error message.
 * Read the ID of the previous last commit from .beargit/.prev
*/
int is_commit_msg_ok(const char* msg) {
  // 'hello GO BEARS bob'
  int i = 0, j = 0;
  while (*(msg + j) != '\0' && (*(go_bears + i)) != '\0') {
    if (*(go_bears + i) != *(msg + j)) {
      j++;
      i = 0;
    } else {
      i++;
      j++;
    }
  }
  if (*(go_bears + i) == '\0')
    return 1;
  else
    return 0;
}

/*
 * Generate the next ID (newid) in such a way that:
 * 1. 40-character strings; all characters are either 6, 1 or c
 * 2. Generating 100 IDs in a row will generate 100 IDs that are all unique
 * 
 * Solution: assume c,1,6 correspong to ternary digits 0,1,2 respectively;
 *           generate id by starting at 0 in ternary and incrementing by 1
 *           for each commit
*/
void next_commit_id(char* commit_id) {
  // initialize commit id to 'c..c' if not found
  if (*commit_id != '6' && *commit_id != '1' && *commit_id != 'c') 
    for (int i = 0; i < COMMIT_ID_SIZE - 1; i++)
      *(commit_id + i) = 'c';
  else {
    int LSB = COMMIT_ID_SIZE - 2;
    // Step 1: convert fake ternary -> decimal
    int i, j;
    unsigned int dec_id = 0; // this number can get big; keep positive
    // loop through commit_id, starting from LSB
    for (i = 0, j = LSB; j >= 0; i++, j--) {
      int tern_digit = pow (3, i);  // find current ternary digit
      switch (*(commit_id + j))     // convert that value as follows:
      {
        case 'c':
          dec_id += tern_digit * 0; // c=0 -> 3^i * 0
          break;
        case '1':
          dec_id += tern_digit * 1; // 1=1 -> 3^i * 1
          break;
        case '6':
          dec_id += tern_digit * 2; // 6=2 -> 3^i * 2
          break;
        default:
          break;
      }
    }
    // Step 2: increment decimal value & reset array iterator
    dec_id++;
    j = LSB;
    // Step 3: convert new dec value to ternary representation
    while (dec_id > 0) {
      int r = dec_id % 3;
      switch (r)
      {
        case 0:
          *(commit_id + j) = 'c';
          break;
        case 1:
          *(commit_id + j) = '1';
          break;
        case 2:
          *(commit_id + j) = '6';
          break;
        default:
          break;
      }
      dec_id = dec_id / 3;
      j--;
    }
  }
}

int beargit_commit(const char* msg) {
  if (!is_commit_msg_ok(msg)) {
    fprintf(stderr, "ERROR: Message must contain \"%s\"\n", go_bears);
    return 1;
  }

  char commit_id[COMMIT_ID_SIZE];
  read_string_from_file(".beargit/.prev", commit_id, COMMIT_ID_SIZE);
  next_commit_id(commit_id);

  // Generate a new directory .beargit/<newid> 
  char commit_dir[FILENAME_SIZE];
  strcpy(commit_dir, ".beargit/");
  strcat(commit_dir, commit_id);
  fs_mkdir(commit_dir);

  // and copy .beargit/.index, .beargit/.prev and all tracked files into the directory.
  char new_index_path[FILENAME_SIZE];
  char new_prev_path[FILENAME_SIZE];
  strcat(strcpy(new_index_path, commit_dir), "/.index");
  strcat(strcpy(new_prev_path, commit_dir), "/.prev");
  fs_cp(".beargit/.index", new_index_path);
  fs_cp(".beargit/.prev", new_prev_path);

  // Store the commit message (<msg>) into .beargit/<newid>/.msg
  char msg_path[FILENAME_SIZE];
  strcat(strcpy(msg_path, commit_dir), "/.msg");
  FILE* msg_fp = fopen(msg_path, "w");  // touch to create file
  fclose(msg_fp);
  write_string_to_file(msg_path, msg);
  
  // Write the new ID into .beargit/.prev.
  write_string_to_file(".beargit/.prev", commit_id);

  return 0;
}

/* beargit status */
int beargit_status() {
  FILE *fp = fopen(".beargit/.index", "r");

  char line[FILENAME_SIZE];
  int count = 0;

  printf("Tracked files:\n\n");
  while (fgets(line, sizeof(line), fp)) {
    printf("  %s", line);
    count++;
  }
  printf("\n%d files total\n", count);

  fclose(fp);

  return 0;
}

/* beargit log
 * List all commits, latest to oldest. 
 *  .beargit/.prev contains the ID of the latest commit, and each directory 
 *  .beargit/ contains a .prev file pointing to that commit's predecessor.
 * 
 * For each commit, print the commit's ID followed by the commit message:
 * [BLANK LINE]
 * commit <ID1>
 *   <msg1>
 * [BLANK LINE]
 * commit <ID2>
 *   <msg2>
 * [...]
 * commit <IDN>
 *   <msgN>
 * [BLANK LINE]
 * 
 * If there are no commits to the beargit repo, 
 * beargit should return 1 and output the following to stderr:
 *  ERROR: There are no commits!
 */

int beargit_log() {
  char current_commit_id[COMMIT_ID_SIZE];

  // read latest commit
  read_string_from_file(".beargit/.prev", current_commit_id, COMMIT_ID_SIZE);

  if (*current_commit_id == '0') { // no latest commit => no commits
    fprintf(stderr, "ERROR: There are no commits!\n");
    return 1;
  }

  char current_commit_msg[MSG_SIZE];
  char prev_commit_id[COMMIT_ID_SIZE];

  while (*current_commit_id != '0') {
    // get pointer to current commit dir
    char current_commit_dir[FILENAME_SIZE];
    strcpy(current_commit_dir, ".beargit/");
    strcat(current_commit_dir, current_commit_id);

    // read msg from current commit dir
    char msg_dir[FILENAME_SIZE];
    strcat(strcpy(msg_dir, current_commit_dir), "/.msg");
    read_string_from_file(msg_dir, current_commit_msg, COMMIT_ID_SIZE);

    // read prev commit id (i.e. next to jump to) from current commit dir
    char prev_commit_dir[FILENAME_SIZE];
    strcat(strcpy(prev_commit_dir, current_commit_dir), "/.prev");
    read_string_from_file(prev_commit_dir, prev_commit_id, COMMIT_ID_SIZE);
    
    // print results
    printf("\n");
    printf("commit %s\n", current_commit_id);
    printf("    %s", current_commit_msg);
    printf("\n");
    
    // update current commit to point to previous commit
    strcpy(current_commit_id, prev_commit_id);
  }

  printf("\n");
  return 0;
}

// ---------------------------------------
// HOMEWORK 2 
// ---------------------------------------

// This adds a check to beargit_commit that ensures we are on the HEAD of a branch.
int beargit_commit(const char* msg) {
  char current_branch[BRANCHNAME_SIZE];
  read_string_from_file(".beargit/.current_branch", current_branch, BRANCHNAME_SIZE);

  if (strlen(current_branch) == 0) {
    fprintf(stderr, "ERROR: Need to be on HEAD of a branch to commit\n");
    return 1;
  }

  return beargit_commit_hw1(msg);
}

const char* digits = "61c";

void next_commit_id(char* commit_id) {
  char current_branch[BRANCHNAME_SIZE];
  read_string_from_file(".beargit/.current_branch", current_branch, BRANCHNAME_SIZE);

  // The first COMMIT_ID_BRANCH_BYTES=10 characters of the commit ID will
  // be used to encode the current branch number. This is necessary to avoid
  // duplicate IDs in different branches, as they can have the same pre-
  // decessor (so next_commit_id has to depend on something else).
  int n = get_branch_number(current_branch);
  for (int i = 0; i < COMMIT_ID_BRANCH_BYTES; i++) {
    // This translates the branch number into base 3 and substitutes 0 for
    // 6, 1 for 1 and c for 2.
    commit_id[i] = digits[n%3];
    n /= 3;
  }

  // Use next_commit_id to fill in the rest of the commit ID.
  next_commit_id_hw1(commit_id + COMMIT_ID_BRANCH_BYTES);
}


// This helper function returns the branch number for a specific branch, or
// returns -1 if the branch does not exist.
int get_branch_number(const char* branch_name) {
  FILE* fbranches = fopen(".beargit/.branches", "r");

  int branch_index = -1;
  int counter = 0;
  char line[FILENAME_SIZE];
  while(fgets(line, sizeof(line), fbranches)) {
    strtok(line, "\n");
    if (strcmp(line, branch_name) == 0) {
      branch_index = counter;
    }
    counter++;
  }

  fclose(fbranches);

  return branch_index;
}

/* beargit branch
 *
 * See "Step 6" in the homework 1 spec.
 *
 */

int beargit_branch() {
  /* COMPLETE THE REST */

  return 0;
}

/* beargit checkout
 *
 * See "Step 7" in the homework 1 spec.
 *
 */

int checkout_commit(const char* commit_id) {
  /* COMPLETE THE REST */
  return 0;
}

int is_it_a_commit_id(const char* commit_id) {
  /* COMPLETE THE REST */
  return 1;
}

int beargit_checkout(const char* arg, int new_branch) {
  // Get the current branch
  char current_branch[BRANCHNAME_SIZE];
  read_string_from_file(".beargit/.current_branch", "current_branch", BRANCHNAME_SIZE);

  // If not detached, update the current branch by storing the current HEAD into that branch's file...
  // Even if we cancel later, this is still ok.
  if (strlen(current_branch)) {
    char current_branch_file[BRANCHNAME_SIZE+50];
    sprintf(current_branch_file, ".beargit/.branch_%s", current_branch);
    fs_cp(".beargit/.prev", current_branch_file);
  }

  // Check whether the argument is a commit ID. If yes, we just stay in detached mode
  // without actually having to change into any other branch.
  if (is_it_a_commit_id(arg)) {
    char commit_dir[FILENAME_SIZE] = ".beargit/";
    strcat(commit_dir, arg);
    if (!fs_check_dir_exists(commit_dir)) {
      fprintf(stderr, "ERROR: Commit %s does not exist\n", arg);
      return 1;
    }

    // Set the current branch to none (i.e., detached).
    write_string_to_file(".beargit/.current_branch", "");

    return checkout_commit(arg);
  }

  // Just a better name, since we now know the argument is a branch name.
  const char* branch_name = arg;

  // Read branches file (giving us the HEAD commit id for that branch).
  int branch_exists = (get_branch_number(branch_name) >= 0);

  // Check for errors.
  if (!(!branch_exists || !new_branch)) {
    fprintf(stderr, "ERROR: A branch named %s already exists\n", branch_name);
    return 1;
  } else if (!branch_exists && new_branch) {
    fprintf(stderr, "ERROR: No branch %s exists\n", branch_name);
    return 1;
  }

  // File for the branch we are changing into.
  char* branch_file = ".beargit/.branch_"; 
  strcat(branch_file, branch_name);

  // Update the branch file if new branch is created (now it can't go wrong anymore)
  if (new_branch) {
    FILE* fbranches = fopen(".beargit/.branches", "a");
    fprintf(fbranches, "%s\n", branch_name);
    fclose(fbranches);
    fs_cp(".beargit/.prev", branch_file); 
  }

  write_string_to_file(".beargit/.current_branch", branch_name);

  // Read the head commit ID of this branch.
  char branch_head_commit_id[COMMIT_ID_SIZE];
  read_string_from_file(branch_file, branch_head_commit_id, COMMIT_ID_SIZE);

  // Check out the actual commit.
  return checkout_commit(branch_head_commit_id);
}
