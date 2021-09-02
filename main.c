#include <stdio.h>
#include<stdlib.h>
#include<string.h>
#include<assert.h>

int cell_dim= 512;
int number_of_transitions;
char end_delimiter = '\0';
char start_delimiter = '1';
int max_state=0;

char** transitions;
unsigned long int     max_steps;
char**  acc_states;

/*
* una hash_cell (alias hash_c) rappresenta una transizione e contiene come attributi
        * lo stato attuale, quello successivo, la mossa da fare sul nastro ( R,L,S), il carattere da scrivere sul
* nastro, e quello che bisogna poter leggere dalla testina sul nastro per far si che la transizione sia "sfruttabile".
* Infine contiene un puntatore ad una hash_c successiva in quanto l'hashing è a concatenamento
*
* esiste un array di hash_c per ogni stato
*/

typedef struct hash_cell{
    int actual_state;
    int next_state;
    char tape_move;
    char write_char;
    char read_char;
    struct hash_cell *next;
}hash_c;

void little_hash_to_printf(hash_c hash){
    printf("\nminghie: %d %c %c %c %d\n",hash.actual_state,hash.read_char,hash.write_char,hash.tape_move,hash.next_state);
}

char*copy_w(const char*word){
    if(word!=NULL) {
        int  l =0;
        while(word[l]!='\0')
            l++;
        char*to_save=malloc(( l +1)* sizeof(char));
        for (int i = 0; i < l; i++) {
            to_save[i] = word[i];
        }
        to_save[l] = '\0';
        return to_save;
    }
    else
        return NULL;
}

//--------------------------------------------------------------------------------------------------------------------//

//inizio funzioni e struct per l'hashing delle transizioni



hash_c *not_exist;

/*
 * questa struct rappresenta un array di hash_c ovvero di celle hash
 */
typedef struct little_hash{
    hash_c **little;
}little_h;

/*
 * rappresenta una hash di hash ovvero un array little_hash che sono a loro volta
 * array di celle hash. Quindi si tratta di un array di array di celle hash alla fine dei conti
 */
typedef struct hash_of_hash{
    char** little_hash_addressing;
    little_h *big_hash_table;
}big_h;

big_h*big_global;

/*
 * data una tr da stdin e un puntatore ad una hash_cell
 */
void decompose_transition(char* transition, hash_c *cell){
    char*tmp =copy_w(transition);
    char*tmp1;
    tmp1 = strtok(tmp," ");
    cell->actual_state=(int) strtol(tmp1,NULL,10);
    tmp1=strtok(NULL," ");
    cell->read_char=  tmp1[0];
    tmp1=strtok(NULL," ");
    cell->write_char=tmp1[0];
    tmp1=strtok(NULL," ");
    cell->tape_move=(tmp1[0]);
    tmp1=strtok(NULL," ");
    cell->next_state=(int) strtol(tmp1,NULL,10);
    free(tmp);
}

/*
 * data una cella della hash e una transizione costruisce una nuova hash_c e la inserisce dopo la testa della lista
 * di hash_c
 */
void append(hash_c *cell, char *transition) {
    if (cell->next_state == -1) {
        decompose_transition(transition,cell);
    }
    else {
        hash_c *new = malloc(sizeof(hash_c));
        decompose_transition(transition,new);
        new->next = cell->next;
        cell->next = new;
    }
}

/*
 * data una tr mi restituisce lo stato attuale della mt ovvero per chiarire meglio ad esempio:
 * return_actual_state("0 a a S 1")  ritorna zero cioè lo stato attuale
 */
int return_actual_state(char*tr){
    char*tmp=copy_w(tr);
    char*tmp1 = strtok(tmp," ");
    int result = (int) strtol(tmp1,NULL,10);
    free(tmp);
    return result;
}

char return_read_char(char*tr){
    char*tmp=copy_w(tr);
    char*tmp1;
    tmp1=strtok(tmp," ");
    tmp1=strtok(NULL," ");
    char result= tmp1[0];
    free(tmp);
    return result;
}

int num_same_read_char_per_state(char** transitions_set){
    int state =return_actual_state(transitions_set [0]);
    char character = return_read_char(transitions_set[0]);
    char character_tmp;
    int i=0,j=1;
    while(return_actual_state(transitions_set[i])==state){
        character_tmp=return_read_char(transitions_set[i]);
        if(character_tmp!=character){
            j++;
            character=character_tmp;
        }
        i++;
    }
    return j+1;
}

char* index_address_word(char** transitions_set){
    int state = return_actual_state(transitions_set[0]);
    char read_char = return_read_char(transitions_set[0]);
    int k = num_same_read_char_per_state(transitions_set);
    char*result = malloc( k* sizeof(char));
    int i=0,j=1;
    result[0]=read_char;
    char tmp;
    while(return_actual_state(transitions_set[i])==state && strcmp(transitions_set[i],"acc")!=0 ){
        tmp=return_read_char(transitions_set[i]);
        if(tmp!=read_char){
            result[j]=tmp;
            j++;
            read_char=tmp;
        }
        i++;
    }
    result[k-1]='\0';
    transitions= transitions+i;
    return result;
}

/*
 * data una "parola di indirizzamento della hash e un carattere mi da la posizione ( partendo da zero)
 * del char nella parola oppure -1 se questo non è contenuto all'interno della parola di indirizzamento
 */
int index_of(char  read_char, const char*addressing){
    int i=0;
    while(addressing[i]!=read_char && addressing[i]!='\0'){
        i++;
    }
    if(addressing[i]=='\0')
        return -1;
    else
        return i;
}

int index_is_final_state(int index){
    int i=0;
    do{
        if(index==strtol(acc_states[i],NULL,10))
            return 1;
        i++;
    }while(strcmp(acc_states[i],"max")!=0);
    return 0;
}

/*
 * costruisce una hash table di hash table di tipo big_h e ne restituisce il puntatore. Se fossimo in un linguaggio
 * ad oggetti si traterebbe del costruttore della classe big_h
 */
big_h* new_big_hash(){
    char**tmp=transitions;
    big_h*big = malloc(sizeof(big_h));
    big->big_hash_table= malloc( (max_state + 1)*sizeof(struct little_hash));
    big->little_hash_addressing=malloc( (max_state + 1)* sizeof(char*));
    char **tmp_addr=big->little_hash_addressing;
    little_h* arr = big->big_hash_table;
    for(int i=0;i<(max_state+1);i++){
        if(!index_is_final_state(i) && i==return_actual_state(*transitions)) {
            tmp_addr[i] = index_address_word(transitions);
            arr[i].little = malloc(strlen(tmp_addr[i])*sizeof(hash_c*) );
            int l= (int) strlen(tmp_addr[i]);
            for(int k=0;k<l;k++){
                arr[i].little[k]=malloc(sizeof(hash_c));
            }
        }
        else{
            tmp_addr[i]="1";
            arr[i].little=NULL;
        }
    }
    transitions=tmp;
    return big;
}

/*
 * riempie la hash grande di tipo big_h
 */
void fill(big_h*big){
    little_h*tmp = big->big_hash_table;
    char** tmp_words = big->little_hash_addressing;
    int length,index,state;
    char read_char;
    for(int i=0;i<max_state+1;i++){
        if(!index_is_final_state(i) && strcmp(tmp_words[i],"1")!=0 ) {
            length = (int) strlen(tmp_words[i]);
            for (int j = 0; j < length; j++) {
                tmp[i].little[j]->next = NULL;
                tmp[i].little[j]->next_state = -1;
            }
        }
    }
    for(int i=0;i<number_of_transitions-1;i++){
        read_char = return_read_char(transitions[i]);
        state = return_actual_state(transitions[i]);
        index = index_of(read_char,tmp_words[state]);
        append((tmp[state].little[index]),transitions[i]);
    }
}

// fine funzioni e struct per l'hashing delle transizioni

//--------------------------------------------------------------------------------------------------------------------//

//funzioni per la singola macchina di turing

/*
 * rappresenta una parte del nastro. Contiene il numero di spazi del nastro non vuoti, un puntatore char (content) rappresentante
 * il pezzo del nastro e infine un puntatore ad un altra parte del nastro a destra di questo ( right) e una alla sinistra (left)
 */
typedef struct cell{
    char*start_content;
    char*content;
    struct cell* right;
    struct cell* left;
} node_cell;


/*
 * rappresenta il nastro che contiene solo la cella a cui mi trovo
 */
typedef struct tape{
    node_cell * actual_cell;
}node_tape;

/*
 * struct di una mt contenente un nastro (tape) una transizione attuale e infine il puntatore alla macchina
 * alla sua destra e sinistra
 */
typedef struct mt{
    node_tape *tape;
    hash_c *actual_tr;
    struct mt* right;
    struct mt* left;
}mt;

mt *all_tms;

/*
 * elimina una cella di un nastro
 */
void delete_cell(node_cell*cell_to_delete){
    if(cell_to_delete->left!=NULL)
            cell_to_delete->left->right=cell_to_delete->right;
    if(cell_to_delete->right!=NULL)
            cell_to_delete->right->left=cell_to_delete->left;
   cell_to_delete->left=NULL;
   cell_to_delete->right=NULL;
   cell_to_delete->content=NULL;
   free(cell_to_delete->start_content);
   free(cell_to_delete);
}

/*
 * fa la free della struct del nastro
 */
void free_tape(node_tape* tape_to_delete){
    node_cell*actual=tape_to_delete->actual_cell,*tmp1,*next;
    tmp1=actual->left;
    while(tmp1!=NULL){
        next=tmp1->left;
        delete_cell(tmp1);
        tmp1=next;
    }
    tmp1=actual->right;
    while(tmp1!=NULL){
        next=tmp1->right;
        delete_cell(tmp1);
        tmp1=next;
    }
    delete_cell(actual);
    free(tape_to_delete);
}

/*
 * fa la free delle struct di una  macchina di turing tra le tante generate
 */
void delete_mt(mt*machine){
    if(machine->right!=NULL)
        machine->right->left=machine->left;
    if(machine->left!=NULL)
        machine->left->right=machine->right;
    free_tape(machine->tape);
    machine->right=NULL;
    machine->left=NULL;
    free(machine);
}

/*
 * fa la free di tutte le macchine di turing generate durante la computazione dell'algoritmo
 */
void delete_all_mt(mt*actual_mt){
    mt*tmp=actual_mt->left,*next;
    while(tmp!=NULL){
        next=tmp->left;
        delete_mt(tmp);
        tmp=next;
    }
    tmp=actual_mt->right;
    while(tmp!=NULL){
        next=tmp->right;
        delete_mt(tmp);
        tmp=next;
    }
    delete_mt(actual_mt);
}


/*
 * costruisce una struct cella settando tutti gli attributi
 */
node_cell* new_cell(){
    int real_dim = cell_dim + 2;
    node_cell * new_cell = malloc(sizeof(node_cell));
    new_cell->content=malloc( real_dim*sizeof(char));
    for(int i=1; i<real_dim-1;i++)
        new_cell->content[i]='_';
    new_cell->start_content=new_cell->content;
    new_cell->content[0] = start_delimiter;
    new_cell->content[real_dim-1] = end_delimiter;
    new_cell->content++;
    new_cell->right = NULL;
    new_cell->left = NULL;
    return new_cell;
}

/*
 * costruttore del nastro
 */
node_tape* new_tape(){
    node_cell *init_cell = new_cell();
    node_tape *tape= malloc(sizeof(node_tape));
    tape->actual_cell=init_cell;
    return tape;
}

int has_next_right(node_cell* actual){
    char next_right_char = *(actual->content+1);
    return next_right_char != end_delimiter;
}

int has_next_left(node_cell* actual){
    char next_left_char = *(actual->content-1);
    return next_left_char != start_delimiter;
}

/*
 * restituisce una puntatore ad una  copia di una node_cell
 */
node_cell*copy_cell(node_cell*original){
    node_cell*copy=malloc(sizeof(node_cell));
    int real_dim = cell_dim+2;
    int i=1;
    char*tmp=original->content;
    while(*(original->content-1)!=start_delimiter){
        original->content--;
        i++;
    }
    copy->content=malloc( real_dim* sizeof(char));
    copy->content[0] = start_delimiter;
    copy->start_content=copy->content;
    copy->content[real_dim-1]=end_delimiter;
    copy->right=NULL;
    copy->left=NULL;
    for(int j=1; j<real_dim-1;j++)
        copy->content[j]=original->content[j-1];
    if(i>0){
        copy->content+=i;
    }
    original->content=tmp;
    return copy;
}

/*
 * copia tutte le node_cell a sinistra della actual_cell del nastro (tape)
 */
node_tape*copy_left(node_tape*original){
    node_tape*copy=malloc(sizeof(node_tape));
    copy->actual_cell = copy_cell(original->actual_cell);
    int i=0;
    while(original->actual_cell->left!= NULL){
        i++;
        copy->actual_cell->left=copy_cell(original->actual_cell->left);
        copy->actual_cell->left->right=copy->actual_cell;
        copy->actual_cell=copy->actual_cell->left;
        original->actual_cell=original->actual_cell->left;
    }
    while(i!=0){
        copy->actual_cell=copy->actual_cell->right;
        original->actual_cell=original->actual_cell->right;
        i--;
    }
    return copy;
}


/*
 * copia tutte le node_cell a destra della actual_cell del nastro (tape)
 */
node_tape *copy_right(node_tape *original, node_tape *copy_left){
    int i=0;
    while(original->actual_cell->right!=NULL){
        i++;
        copy_left->actual_cell->right=copy_cell(original->actual_cell->right);
        copy_left->actual_cell->right->left=copy_left->actual_cell;
        copy_left->actual_cell=copy_left->actual_cell->right;
        original->actual_cell=original->actual_cell->right;
    }
    while(i!=0){
        copy_left->actual_cell=copy_left->actual_cell->left;
        original->actual_cell=original->actual_cell->left;
        i--;
    }
    return copy_left;
}

/*
 * funzione che fa la copia del nastro
 */
node_tape*copy(node_tape*original){
    node_tape*copy=copy_left(original);
    copy=copy_right(original,copy);
    return copy;
}

/*
 * funzione che fa la copia di una mt e inserisce la copia alla destra della macchina da duplicare
 */
void duplicate_mt(mt*mt_to_duplicate){
    mt*copied=malloc(sizeof(mt));
    copied->tape=copy(mt_to_duplicate->tape);
    copied->right=mt_to_duplicate->right;
    if(mt_to_duplicate->right!=NULL)
        mt_to_duplicate->right->left=copied;
    copied->left=mt_to_duplicate;
    mt_to_duplicate->right=copied;
}

/*
 * posiziona la testina del nastro di una posizione più a destra rispetto ad actual_cell.
 * Per fare ciò si sposta a destra facendo il right di actual_cell e se right esiste in quanto diverso
 * da null allora restituisce right altrimenti essendo alla fine del nastro verso destra crea una nuova cell
 * la inserisce a destra del nastro e la restituisce
 */
node_cell* go_right(node_cell* actual_cell){
    if(has_next_right(actual_cell)){
        actual_cell->content++;
        return actual_cell;
    }
    else{
        if(actual_cell->right!=NULL){
            return actual_cell->right;
        }
        else {
            node_cell *new_actual = new_cell();
            new_actual->left = actual_cell;
            new_actual->right = NULL;
            actual_cell->right = new_actual;
            return new_actual;
        }
    }
}


/* posiziona la testina del nastro di una posizione più a sinistra rispetto ad actual_cell.
* Per fare ciò si sposta a sinistra facendo il left di actual_cell e se left esiste in quanto diverso
* da null allora restituisce left altrimenti essendo alla fine del nastro (verso sinistra) crea una nuova cell
* la inserisce a sinistra del nastro e la restituisce. La nuova cella restituita diventa la nuova actual_tape_cell
 * di mt
*/
node_cell* go_left(node_cell* actual_cell ){
    if(has_next_left(actual_cell)){
        actual_cell->content--;
        return actual_cell;
    }
    else{
        if(actual_cell->left!=NULL){
            return actual_cell->left;
        }
        else {
            node_cell *new_actual = new_cell();
            // va all'ultimo puntatore a char della della stringa cioè:
            new_actual->content+=(cell_dim-1);
            new_actual->left = NULL;
            new_actual->right = actual_cell;
            actual_cell->left = new_actual;
            return new_actual;
        }
    }
}

/*
 * con questa funzione si scrive il contenuto new_content
 * nella cella attuale del nastro di memoria della mt deterministica
 */
node_cell* write_on_tape(node_cell* actual_cell, char new_content){
    *actual_cell->content=new_content;
    return actual_cell;
}
/*
 * data una parola (word) costruisce la struttura per una macchine di turing inserendo la parola nel nastro e posizionando la testina
 * sulla prima lettera della parola in questione una volta posizionata
 */
mt* new_mt(char *word){
    mt*machine=malloc(sizeof(mt));
    machine->tape=new_tape();
    int l=(int)strlen(word);
    node_cell **tmp_cell= malloc(sizeof(node_cell*));
    *tmp_cell=machine->tape->actual_cell;
    char*tmp_content=(*tmp_cell)->content;
    for(int i=0; i<l;i++){
        machine->tape->actual_cell=write_on_tape(machine->tape->actual_cell,word[i]);
        machine->tape->actual_cell=go_right(machine->tape->actual_cell);
    }
    machine->tape->actual_cell=(*tmp_cell);
    machine->tape->actual_cell->content=tmp_content;
    while((*tmp_cell)!=NULL){
        (*tmp_cell)->content=(*tmp_cell)->start_content+1;
        (*tmp_cell)=(*tmp_cell)->right;
    }
    machine->right=NULL;
    machine->left=NULL;
    *tmp_cell=NULL;
    free(tmp_cell);
    return machine;
}


/*
 * dato il char sul nastro (sulla testina) e lo stato successivo in cui andrà la macchina di turing
 * mi ritorna una lista di transizioni abilitate in quel caso. Se la lista è formata solo dalla testa (da una sola hash_cell) allora
 * ho determinismo nella scelta altrimenti, se ciò non succede vuol dire che più transizioni sono abilitate e che quindi si
 * ha non determinismo in questo caso. Il tempo per la restituizione della lista è costante.
 */
hash_c * next(char char_tape, int state){
    hash_c*result;
    little_h*tmp = big_global->big_hash_table;
    char** tmp_words = big_global->little_hash_addressing;
    if(state<=max_state  && tmp_words[state]!=NULL) {
        int index = index_of(char_tape, tmp_words[state]);
        if(index==-1)
            return not_exist;
        result = tmp[state].little[index];
        return result;
    }
    else
        return not_exist;
}

/*
 * costruisce "manualmente" il primo livello dell'albero
 */
int init(char *word){
    all_tms=new_mt(word);
    mt*tmp=all_tms;
    tmp->right=all_tms->right;
    tmp->left=all_tms->left;
    tmp->actual_tr=all_tms->actual_tr;
    tmp->tape=all_tms->tape;
    hash_c*next_tr=next(*all_tms->tape->actual_cell->content,0);
    if(next_tr!=not_exist) {
        hash_c *tmp_tr = next_tr;
        if (tmp_tr->next == NULL) {
            all_tms->actual_tr = next_tr;
        } else {
            while (tmp_tr != NULL) {
                if (tmp_tr->next != NULL)
                    duplicate_mt(tmp);
                tmp->actual_tr = tmp_tr;
                if (tmp_tr->next != NULL)
                    tmp = tmp->right;
                tmp_tr = tmp_tr->next;
            }
        }
        return 1;
    } else {
        delete_all_mt(all_tms);
        return 0;
    }
}

/*
 * funzione che data una mt fa evolvere ( fa la mossa) il nastro  ma non cambia stato
 */
void execute_det_move(mt*machine){
    machine->tape->actual_cell=write_on_tape(machine->tape->actual_cell,machine->actual_tr->write_char);
    char tape_move= machine->actual_tr->tape_move;
    if(tape_move=='R')
        machine->tape->actual_cell=go_right(machine->tape->actual_cell);
    else {
        if (tape_move == 'L')
            machine->tape->actual_cell = go_left(machine->tape->actual_cell);
    }
}

int  exist(mt**actual,hash_c*nexts_tr){
    hash_c**tmp= malloc(sizeof(hash_c *));
    *tmp=nexts_tr;
    while(*tmp!=NULL){
        if((*tmp)->next!=NULL)
            duplicate_mt((*actual));
        (*actual)->actual_tr=*tmp;
        if((*tmp)->next!=NULL)
            (*actual)=(*actual)->right;
        (*tmp)=(*tmp)->next;
    }
    *tmp=NULL;
    free(tmp);
    return (*actual)->right!=NULL;
}

/*
 * data una mt fa la mossa del nastro e poi successivamente trova l'insieme delle mosse che sono applicabili
 * su quella mt. Se c'è solo una mossa applicabile allora ho determinismo altrimenti ho non determinismo.
 * Se ho determinismo associa la nuova mossa alla macchina e non duplica niente altrimenti in caso di mossa
 * non deterministica duplica la macchina e assegna la nuova mossa alla macchina in questione e si sposta a destra
 * dove è presente la macchina precedentemente duplicata. Fa tutto ciò per il numero di mosse applicabili
 */
int next_mt(mt**actual) {
    int first_result;
    execute_det_move(*actual);
    hash_c*next_tr=next((*(*actual)->tape->actual_cell->content),(*actual)->actual_tr->next_state);
    if(next_tr!=not_exist){
        first_result=exist(actual,next_tr);
        if(first_result)
            return -1;
        else
            return -2;
    }
    else{
        return 3;
        }
}

int delete_or_refuse(mt**actual,mt**head){
    mt*next_mt_n;
    if(index_is_final_state((*actual)->actual_tr->next_state)){
        printf("1");
        delete_all_mt((*actual));
        return 1;
    }
    if((*actual)->right==NULL && (*actual)->left== NULL && (*actual)==(*head)) {
        printf("0");
        delete_mt(*actual);
        return 0;
    }
    if((*actual)==(*head)){
        next_mt_n=(*actual)->right;
        delete_mt((*actual));
        (*actual)=next_mt_n;
        (*head)=(*actual);
        return -1;
    }
    else{
        if((*actual)->right!=NULL){
            next_mt_n=(*actual)->right;
            delete_mt((*actual));
            (*actual)=next_mt_n;
            return -1;
        }else{
            next_mt_n=(*head);
            delete_mt((*actual));
            (*actual)=next_mt_n;
            return -2;
        }
    }
}

/*
 * funzione che dal livello precedente dell'albero delle computazioni genera il livello successivo. In memoria ad ogni passo
 * sarà presente solo il livello corrente e non quello precedente
 */
int next_computation_tree_level(mt**actual,mt**head){
    int first_result,second_result;
    while(1){
        first_result=next_mt(actual);
        if(first_result==3) {
            second_result = delete_or_refuse(actual, head);
            if (second_result != -1) {
                if (second_result == 1 || second_result == 0)
                    return 1;
                if (second_result == -2)
                    return 0;
            }
        }
        else{
            if(first_result==-1)
                (*actual)=(*actual)->right;
            else{
                (*actual) = (*head);
                return 0;
            }
        }
    }
}

int analyze_last_level(mt**actual){
    int one_able=0;
    hash_c **tmp= malloc(sizeof(hash_c*));
    do{
        execute_det_move(*actual);
        *tmp= next(*(*actual)->tape->actual_cell->content,(*actual)->actual_tr->next_state);
        if(*tmp!=not_exist)
            one_able=1;
        else{
            if(index_is_final_state((*actual)->actual_tr->next_state))
                return 1;
        }
        if((*actual)->right!=NULL)
            (*actual)=(*actual)->right;
        else
            break;
    }while(1);
    *tmp=NULL;
    free(tmp);
    if(one_able==1)
        return -1;
    else
        return 0;
}

/*
 * funzione che computa una stringa dandone i risultati (0,1,U)
 */
void compute(char*word){
    int a,result=0;
    a=init(word);
    mt**actual=malloc(sizeof(mt*));
    *actual=all_tms;
    mt**head=malloc(sizeof(mt*));
    *head=all_tms;
    int b;
    unsigned long int steps=0;
    if(a==1){
        while(result != 1){
            steps++;
            if(steps==max_steps){
                b = analyze_last_level(actual);
                if(b==-1){
                    printf("U");
                    delete_all_mt(*actual);
                    break;
                }
                if(b==1){
                    printf("1");
                    delete_all_mt(*actual);
                    break;
                }if(b==0){
                    printf("0");
                    delete_all_mt(*actual);
                    break;
                }

        }
            result=next_computation_tree_level(actual,head);
        }
    }else{
        printf("0");
    }
    free(head);
    free(actual);
}



//--------------------------------------------------------------------------------------------------------------------//

//funzioni per generare le chiavi delle transizioni e per confrontarle

/*
 * data una lettera da come risultato la chiave della lettera ottenuto come (valore ascii -96) oppure 0 se il
 * carattere in questione è '_'
 */
int alpha_key(char letter){
    if(letter=='_')
        return 0;
    else
        return ((int) letter);
}

/*
 * data una tranzione (tr) mi da come risultato la sua relativa chiave ottenuta come stato attuale*100 sommato
 * al valore di alpha_key(char da leggere)
 */
long int transition_key(char *transition){
    long int transition_key;
    char*tmp_tr = copy_w(transition);
    char*tmp = strtok(tmp_tr," ");
    transition_key = ((int) strtol(tmp, NULL, 10))*1000;
    tmp = strtok(NULL," ");
    transition_key+= alpha_key( tmp[0]);
    free(tmp_tr);
    return transition_key;
}

/*
 * ritorna 1 se la tr tr1 è minore o uguale della tr tr2 altrimento ritorna zero. Si avvale delle funzioni per le chiavi
 * per fare ciò
 */
int is_minor(char* tr1, char*tr2){
    if(tr1[0]=='_')
        return 0;
    if(tr2[0]=='_')
        return 1;
    else
        return transition_key(tr1)<=transition_key(tr2);
}
//fine gruppo di funzioni per le chiavi

//--------------------------------------------------------------------------------------------------------------------//

//inizio funzioni riguardo all'ordinamento delle transizioni

/*
 * classica funzione di merge per il merge sort
 * Funziona con un array di tr avvalendosi delle funzioni per la generazione e confronto di chiavi per l'ordinamento
 */
void merge (char** a, int low, int medium, int high ){
    int n1= medium-low+1;
    int n2= high-medium;
    char** L = calloc(((size_t)n1+1),sizeof(char*));
    char** R = calloc( ((size_t)n2 + 1), sizeof(char*));
    for(int i=0;i<n1;i++) {
        L[i] = a[i + low];
    }
    for(int j=0; j<n2;j++) {
        R[j] = a[medium + j+1];
    }
    L[n1]="_ _ _ _ _";
    R[n2]="_ _ _ _ _";
    int i=0;
    int j=0;
    for(int k=low; k<=high;k++){
        if(is_minor(L[i],R[j])){
            a[k]=L[i];
            i++;
        }
        else {
            a[k] = R[j];
            j++;
        }
    }
    free(L);
    free(R);
}

/*
 * classica funzione di mergesort per l'array di transizioni (tr)
 */
void merge_Sort (char** transitions, int left, int right) {
    int center;
    if (left < right) {
        center = (right+left) / 2;
        merge_Sort(transitions, left, center);
        merge_Sort(transitions, center + 1, right);
        merge(transitions, left, center, right);
    }
}

/*
 * funzione che richiama il mergesort sull'array di transizioni (tr) per ordinarlo
 */
void order(){
    merge_Sort(transitions,0,number_of_transitions-2);
}

//--------------------------------------------------------------------------------------------------------------------//

/*
 * si tratta di un tipo strcmp ma NULL safe. Se s1=NULL e s2=NULL restituisce 0,
 * se uno di entrambi è NULL ma l'altro non lo è restituisce 1 e altrimenti
 * restituisce strcmp(s1,s2)
 */
int cmp(const char*s1, const char*s2){
    if(s1==NULL && s2==NULL)
        return 0;
    if((s1==NULL) || (s2==NULL))
        return 1;
    else {
        int i = 0;
        while(s1[i]!='\0'&& s2[i]!='\0'){
            if(s1[i]!=s2[i])
                return 1;
            i++;
        }
        if(s1[i]=='\0'&& s2[i]=='\0')
            return 0;
        else
            return 1;
    }
}

/*incremento delle dimensioni di pointer di increment posizioni. Dopo questo metodo pointer avrà dimensioni pari
 * a dim+increment
 */
char* large(char*pointer, int dim, int increment){
    int new_dim= dim+increment;
    char*tmp;
    do {
        tmp = realloc(pointer, new_dim* sizeof(char));
    }while(tmp==NULL);
    return tmp;
}

/*
 * legge dinamicamente una linea e restituisce il puntatore al primo carattere delle linea letto.
 * Il buffer di lettura è gestito in maniera dinamica
 */
char* read_line(int flag){
    int init_length = 4;
    char*buffer = calloc((size_t) init_length, sizeof(char));
    char c;
    int length=0;
    int number_of_increments = 1 ;
    while((c= (char) fgetc(stdin)) != EOF) {
        if(length+1 == number_of_increments * init_length){
            number_of_increments++;
            buffer = large(buffer,length+1,init_length);
        }
        buffer[length] = c;
        length++;
        if(flag==1 && c==' ')
            break;
        if(c=='\0')
            break;
        if(c=='\n') {
            length--;
            break;
        }
    }
    if(length==0){
        free(buffer);
        return NULL;
    }
    else {
        char*tmp;
        do{
            tmp = realloc(buffer, (length+1) *sizeof(char));

        }while(tmp==NULL);
        tmp[length]='\0';
        return tmp;
    }
}

/*
 * setta il valore i max state a new_max
 */
void set_max(int new_max){
    max_state=new_max;
}

/*incremento delle dimensioni di pointer di increment posizioni. Dopo questo metodo pointer avrà dimensioni pari
 * a dim+increment
 */
char** large_double_pointer(char **pointer,int dim, int increment){
    int new_dim= dim+increment;
    char**tmp;
    do {
        tmp = (char**)realloc(pointer, new_dim * sizeof(char*));
    }while(tmp==NULL);
    return tmp;
}

/*
 * setta il valore di number_of_transition a number
 */
void set_number_of_transition(int number){
    number_of_transitions=number;
}

void set(char*buffer){
    if(buffer!=NULL) {
        char *tmp1 = copy_w(buffer);
        char *tmp2 = strtok(tmp1, " ");
        int *a = malloc(sizeof(int));
        *a = (int) strtol(tmp2, NULL, 10);
        if (*a > max_state)
            set_max(*a);
        free(a);
        free(tmp1);
    }
}

/*
 * dato un delimitatore la funzione parse restituisce un doppio puntatore contenenti stringhe lette fino a quel punto
 * fino a quando non si presenta delimiter tra i caratteri letti. A quel punto restituisce il doppio puntatore contenente
 * tutte le parole lette e come ultimo elemento pure delimiter. La gestione della memoria è sempre dinamica.
 */
char** parse(char* delimiter, int choice){
    int init_dim = 8;
    int length=0;
    int num_of_increment = 1;
    char** buffer = malloc(sizeof(char*)*init_dim);
    do{
            buffer[length] = read_line(0);
        if(choice==1 &&buffer[length]!=NULL) {
            set(buffer[length]);
        }
        length++;
        if(length == num_of_increment*init_dim){
            num_of_increment++;
            buffer = large_double_pointer(buffer,length,init_dim);
        }
    }while(cmp(buffer[length-1],delimiter)!=0);
    char**tmp;
    do{
        tmp = realloc(buffer,(length)*sizeof(char*));
    }while(tmp==NULL);
    if(choice) {
        set_number_of_transition(length);
    }
    return tmp;
}

/*
 * legge l'input e lo parsa nei relativi array dichiarati come variabili globali
 */
void parsing_set_Up(){
    char* tmp;
    tmp=read_line(0);
    free(tmp);
    transitions = parse("acc",1);
    acc_states  = parse("max",0);
    char*tmp1=read_line(0);
    max_steps = (unsigned long) strtol(tmp1,NULL,10);
    free(tmp1);
    char*tmp2=read_line(0);
    free(tmp2);
}

/*
 * Questa funzione stampa gli attributi di una cella hash (cioè una transizione) che viene passata come input.
 * Funzione utile per debuggare le celle della hash e nient'altro.
 */


// fine delle funzioni di setUp
//--------------------------------------------------------------------------------------------------------------------//


/*
 * funzione che fa la free del doppio puntatore delle transizioni. La funzione viene usata dopo aver creato
 * e riempito la hash table di hash table in quanto non più necessaria.
 */
void free_tr(){
    for(int i=0;i<number_of_transitions;i++){
        free(transitions[i]);
    }
    free(transitions);
}

void printf_words(){
    char*word;
    int i=0;
    do{
        i++;
        word=read_line(1);
        //scanf("%ms",word);
        if(word==NULL)
            break;
        printf("\n%d parola: %s, lunghezza: %zu\n",i,word,strlen(word));
        free(word);
    }while(1);
}

/*
 * progettoDiApi main()
 */
int main() {
    //freopen("/home/osboxes/CLionProjects/Api_project/pubblico.txt", "r", stdin);
    //printf_words();
    parsing_set_Up();
    order();
    big_global=new_big_hash();
    fill(big_global);
    not_exist=malloc(sizeof(hash_c));
    not_exist->next_state=-1;
    free_tr();
    char*word;
    while((word=read_line(1))!=NULL){
        if(word[0]==' ')
            break;
        compute(word);
        printf("\n");
        free(word);
        all_tms=NULL;
    }
    return 0;
}