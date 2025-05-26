#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#define MAX_PROC 10 // 프로세스의 최대 개수
#define MAX_QUEUE 10 // 큐 크기
#define MAX_ARRIVAL 20 // arrival time 0 ~ 19
#define MAX_BURST 20 // burst time 1 ~ 20
#define SCHEDUER 6 // 스케쥴링 알고리즘 개수

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


int waiting_time[SCHEDUER], turnaround_time[SCHEDUER];

//int no_print = 0; // 간트 차트 필요 없을 때 사용. 혹시 필요하면 활성화하자

// creat_process 파트
void Creat_Process(void) {
    char choice;

    // 프로세스 수 입력
    do {
        printf("Enter number of processes (1~%d): ", MAX_PROC);
        scanf("%d", &process_num);
    } while (process_num < 1 || process_num > MAX_PROC);

    // 랜덤 생성 여부 선택
    do {
        printf("Random creation? Y/N: ");
        scanf(" %c", &choice);
    } while (choice != 'Y' && choice != 'y' && choice != 'N' && choice != 'n');

    if (choice == 'Y' || choice == 'y') {
        // 랜덤 생성
        
        srand((unsigned)time(NULL)); // 난수 시드 초기화
        
        for (int i = 0; i < process_num; i++) {
            ori[i].pid          = i + 1;
            ori[i].arrival_time = rand() % MAX_ARRIVAL;      // 0 ~ 19
            ori[i].burst_time   = (rand() % MAX_BURST) + 1;   // 1 ~ 20
            ori[i].priority     = (rand() % process_num) + 1;// 1 ~ process_num
            ori[i].remaining    = ori[i].burst_time;
            p[i] = ori[i]; // 복사본 생성
        }
    } else {
        // 수동 입력
        for (int i = 0; i < process_num; i++) {
            ori[i].pid = i + 1;
            printf("Enter arrival time, CPU burst time, priority for [process %d]: ", ori[i].pid);
            scanf("%d %d %d", &ori[i].arrival_time, &ori[i].burst_time, &ori[i].priority);
            ori[i].remaining = ori[i].burst_time;
            p[i] = ori[i];
        }
    }

    // 생성 결과 출력
    printf("\n[Process List After Creation]\n");
    printf("PID\tAT\tBT\tPR\tREM\n");
    for (int i = 0; i < process_num; i++) {
        printf("P%-2d\t%d\t%d\t%d\t%d\n",
               ori[i].pid,
               ori[i].arrival_time,
               ori[i].burst_time,
               ori[i].priority,
               ori[i].remaining);
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

void copy_process(void) {
    for(int i = 0; i < process_num; i++)
        p[i] = ori[i];
}


// start부터 end까지 idx 번째 프로세스가 CPU 사용
void print_Gantt(int start, int end, int idx) {
    if (idx < 0) {
        // IDLE 구간
        printf("| %2d - %2d : IDLE\n", start, end);
    } else {
        // 프로세스 실행 구간
        printf("| %2d - %2d : P%d\n", start, end, idx);
    }
}

void FCFS(Queue *readyQ, Queue *waitQ) {
    int finished = 0;
    int time = 0;
    waiting_time[0] = 0;
    turnaround_time[0] = 0;

    // 초기화
    copy_process();
    Config(readyQ, waitQ);

    printf("\n--- FCFS Scheduling ---\n");

    
    // 시간 남으면 flag를 사용해서 enqueue 됐다? 그러면 패스 이런 식으로 enqueue 다 한 다음에 dequeue 루프 도는 방식 해보자.
    while (finished < process_num) {
        // 다음 실행할 프로세스 찾기
        int idx = -1; // 일단 후보가 없어.
        for (int i = 0; i < process_num; i++) {
            if (p[i].remaining == 0) continue; // 이미 처리된 프로세스
            if (idx == -1 || p[i].arrival_time < p[idx].arrival_time)
                idx = i;
        }

        // idx 로 enqueue → dequeue / 굳이 큐 거칠 필요가 있나..? 구현 했으니 일단...
        Enqueue(readyQ, p[idx]);
        process cur = Dequeue(readyQ);

        // idle 처리
        if (cur.arrival_time > time) {
            print_Gantt(time, cur.arrival_time, -1);
            time = cur.arrival_time;
        }

        // 프로세스 실행
        print_Gantt(time, time + cur.burst_time, cur.pid);

        // wt, tat 총계 누적
        waiting_time[0]    += (time - cur.arrival_time);
        turnaround_time[0] += (time + cur.burst_time - cur.arrival_time);

        // 완료 되면 bt 만큼 time 옮겨주고 remaining 0으로, 처리된 거 하나 추가.
        time += cur.burst_time;
        p[idx].remaining = 0;
        finished++;
    }
}

void SJF(Queue *readyQ, Queue *waitQ) {
    int time = 0;  // 현재 시간
    int finished = 0;  // 완료된 프로세스 수

    waiting_time[1] = 0;
    turnaround_time[1] = 0;

    // 초기화
    copy_process();
    Config(readyQ, waitQ);

    printf("\n--- SJF Scheduling ---\n");

    while (finished < process_num) {
        int idx = -1; // 우선 X

        // time 시점에 도착한 프로세스 중에서 bt가 가장 짧은 프로세스 찾아
        for (int i = 0; i < process_num; i++) {
            if (p[i].remaining == 0) continue; //
            if (p[i].arrival_time <= time) { // ****
                if (idx == -1 || p[i].burst_time < p[idx].burst_time || (p[i].burst_time == p[idx].burst_time
                        && p[i].arrival_time < p[idx].arrival_time))
                {
                    idx = i;
                }
            }
        }

        // 아직 도착한 게 없으면, 앞으로 제일 먼저 도착할 놈 찾기
        if (idx == -1) {
            for (int i = 0; i < process_num; i++) {
                if (p[i].remaining == 0) continue;
                if (idx == -1 || p[i].arrival_time < p[idx].arrival_time || (p[i].arrival_time == p[idx].arrival_time
                        && p[i].burst_time < p[idx].burst_time))
                {
                    idx = i;
                }
            }
            // idle 구간
            print_Gantt(time, p[idx].arrival_time, -1);
            time = p[idx].arrival_time;
        }

        // enqueue → dequeue / 얘도 굳이 필요 있을까..? time quantum 존재하는 거 아니면 진짜 필요 없을 거 같어
        Enqueue(readyQ, p[idx]);
        process cur = Dequeue(readyQ);

        // 실행 간트
        print_Gantt(time,time + cur.burst_time, cur.pid);

        // wt, tat
        waiting_time[1]    += (time - cur.arrival_time);
        turnaround_time[1] += (time + cur.burst_time - cur.arrival_time);

        // 상태 업데이트
        time += cur.burst_time;
        p[idx].remaining = 0;
        finished++;
    }
}

void Priority(Queue *readyQ, Queue *waitQ) {
    int time = 0;  // 현재 시간
    int finished = 0;  // 완료된 프로세스 수

    waiting_time[2] = 0;
    turnaround_time[2] = 0;

    // 초기화
    copy_process();          // ori[] → p[]
    Config(readyQ, waitQ);   // 큐 초기화

    printf("\n--- Priority Scheduling ---\n");

    while (finished < process_num) {
        int idx = -1;

        // time 시점에 도착한 애들 중 우선순위가 가장 높은 놈 찾기
        for (int i = 0; i < process_num; i++) {
            if (p[i].remaining == 0) continue;
            if (p[i].arrival_time <= time) {
                if (idx == -1 || p[i].priority > p[idx].priority || (p[i].priority == p[idx].priority
                        && p[i].arrival_time < p[idx].arrival_time))
                {
                    idx = i;
                }
            }
        }

        // 아직 도착한 프로세스가 없으면, 도착할 놈 찾기
        if (idx == -1) {
            for (int i = 0; i < process_num; i++) {
                if (p[i].remaining == 0) continue;
                if (idx == -1 || p[i].arrival_time < p[idx].arrival_time || (p[i].arrival_time == p[idx].arrival_time
                        && p[i].priority > p[idx].priority))
                {
                    idx = i;
                }
            }
            print_Gantt(time, p[idx].arrival_time, -1);
            time = p[idx].arrival_time;
        }

        // enqueue → dequeue
        Enqueue(readyQ, p[idx]);
        process cur = Dequeue(readyQ);

        // 프로세스 실행
        print_Gantt(time, time + cur.burst_time, cur.pid);

        // wt, tat
        waiting_time[2] += (time - cur.arrival_time);
        turnaround_time[2] += (time + cur.burst_time - cur.arrival_time);

        // 끝 -> 업데이트
        time += cur.burst_time;
        p[idx].remaining = 0;
        finished++;
    }
}



int main(void) {
    Queue readyQ, waitQ;

    // 프로세스 생성
    Creat_Process();
    

    // FCFS 스케줄러 실행
    FCFS(&readyQ, &waitQ);

    // FCFS's wt, tat
    float avg_wt  = (float)waiting_time[0]    / process_num;
    float avg_tat = (float)turnaround_time[0] / process_num;

    printf("\n--- FCFS Summary ---\n");
    printf("Average Waiting Time   : %.2f\n", avg_wt);
    printf("Average Turnaround Time: %.2f\n", avg_tat);
    
    // SJF - Nonpreemptive
    SJF(&readyQ, &waitQ);

    avg_wt  = (float)waiting_time[1]    / process_num;
    avg_tat = (float)turnaround_time[1] / process_num;
    printf("\n--- SJF Summary ---\n");
    printf("Average Waiting Time   : %.2f\n", avg_wt);
    printf("Average Turnaround Time: %.2f\n", avg_tat);
    
    Priority(&readyQ, &waitQ);

    avg_wt  = (float)waiting_time[2]    / process_num;
    avg_tat = (float)turnaround_time[2] / process_num;
    printf("\n--- Priority Summary ---\n");
    printf("Average Waiting Time   : %.2f\n", avg_wt);
    printf("Average Turnaround Time: %.2f\n", avg_tat);
    
    
    return 0;
}
