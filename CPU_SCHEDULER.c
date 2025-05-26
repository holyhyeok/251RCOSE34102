#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#define MAX_PROC 10 // 프로세스의 최대 개수
#define MAX_QUEUE 10 // 큐 크기
#define MAX_ARRIVAL 20 // arrival time 0 ~ 19
#define MAX_BURST 20 // burst time 1 ~ 20

// I/O operation 제외한 process 구조체
typedef struct {
    int pid;
    int arrival_time;
    int burst_time;
    int priority;
    int remaining; // 남은 실행시간
} process;

// 큐 노드 정의
typedef struct Node {
    process data;
    struct Node *next;
} Node;

// 큐 헤더
typedef struct {
    Node *head;
    Node *tail;
    int count;
} Queue;




process ori[MAX_PROC]; //프로세스들의 정보를 담는 원본 배열
process p[MAX_PROC]; // 스케쥴링 단계에서 ori로부터 값을 복사받아 스케쥴링 과정에 직접 사용
int process_num; // 프로세스의 개수

// creat_process 파트
void Creat_Process(void) {
    
    do {
        printf("Enter number of processes(1 ~ %d): ", MAX_PROC);
        scanf("%d", &process_num);
    } while (process_num < 1 || process_num > MAX_PROC);
    
    srand((unsigned)time(NULL)); //난수 시드 초기화
    
    for (int i = 0; i < process_num; i++) {
        ori[i].pid = i + 1;
        ori[i].arrival_time = rand() % MAX_ARRIVAL; //0 ~ 19
        ori[i].burst_time = (rand() % MAX_BURST) + 1; // 1 ~ 20
        ori[i].priority = (rand() % process_num) + 1; //1 ~ process_num
        ori[i].remaining = ori[i].burst_time;

        // p 구조체에 ori에 생성된 process 정보 복사하기
        p[i] = ori[i];
    }
    
    printf("\n[Randomly Generated Processes]\n");
    printf("PID\tAT\tBT\tPR\n"); //pid, arrival time, burst time, priority
    for (int i = 0; i < process_num; i++) {
        printf("P%-2d\t%d\t%d\t%d\n",
               ori[i].pid,
               ori[i].arrival_time,
               ori[i].burst_time,
               ori[i].priority);
    }
    
    
}

// config 파트 - queue는 linked list로 관리

// init Queue - 큐 초기화
void InitQueue(Queue *q) {
    q->head = NULL;
    q->tail = NULL;
    q->count = 0;
}


// 두 개의 큐 초기화
void Config(Queue *readyQ, Queue *waitQ) {
    InitQueue(readyQ);
    InitQueue(waitQ);
}

// ready queue가 비었는지 확인
int IsEmpty(Queue *q) {
    return q->count == 0;
}

// ready_enqueue
int Enqueue(Queue *q, process pr) {
    // 큐가 다 찼으면?
    if (q->count >= MAX_QUEUE) {
        printf("Error: Queue is full (max %d). Cannot enqueue P%d\n", MAX_QUEUE, pr.pid);
        return -1; // 실패
    }
    
    Node *node = malloc(sizeof(Node));
    
    if (!node) return -1; // 메모리 할당 실패
    
    // 메모리 할당 성공
    node->data = pr;
    node->next = NULL;
    
    // 빈 리스트에 삽입
    if (IsEmpty(q)) {
        q->head = node;
        q->tail = node;
    } else { // 한 개 이상 노드가 이미 있는 경우
        q->tail->next = node;
        q->tail = node;
    }
    
    q->count++;
    
    return 0; // 삽입 성공
    
}

process Dequeue(Queue *q) {
    if (q->count == 0) {
        fprintf(stderr, "Error: Cannot dequeue from empty queue\n");
        exit(EXIT_FAILURE);
    }
    
    Node *node = q->head;
    process pr_out = node->data;
    q->head = node->next;
    
    if (q->head == NULL) { //하나 빼니까 가르킬 게 없을 때.
        q->tail = NULL;
    }
    free(node);
    q->count--;
    
    return pr_out;
}



int main(void) {
    Queue readyQ, waitQ;
    // 1) 큐 초기화
    Config(&readyQ, &waitQ);

    // 2) 프로세스 생성
    Creat_Process();  // ori[]와 p[]에 랜덤 프로세스 채워짐.

    // 3) enqueue 테스트
    for (int i = 0; i < process_num; i++) {
        if (Enqueue(&readyQ, p[i]) == 0) {
            printf("Enqueued P%d\n", p[i].pid);
        }
    }

    // 4) dequeue 테스트
    printf("\nDequeuing all processes:\n");
    while (!IsEmpty(&readyQ)) {
        process cur = Dequeue(&readyQ);
        printf("Dequeued P%d (AT=%d, BT=%d, PR=%d)\n",
               cur.pid, cur.arrival_time, cur.burst_time, cur.priority);
    }

    return 0;
}
