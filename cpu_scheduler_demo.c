#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <stdbool.h>

#define MAX_PROC 10 // 프로세스의 최대 개수
#define MAX_QUEUE 10 // 큐 크기
#define MAX_ARRIVAL 15 // arrival time 0 ~ 14
#define MAX_BURST 15 // burst time 1 ~ 15
#define SCHEDUER 6 // 스케쥴링 알고리즘 개수

// I/O operation 제외한 process 구조체
typedef struct {
    int pid;
    int arrival_time;
    int burst_time;
    int priority;
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

char *match[SCHEDUER+1] = {"FCFS", "SJF", "Priority", "RR", "preemptive SJF", "preemptive Priority", "Exit"};

int waiting_time[SCHEDUER], turnaround_time[SCHEDUER];

int no_print = 0; // evaluation 할 때에는 간트 차트 필요 X


/**생성된 프로세스를 알고리즘 돌리기 전에 출력하는 함수**/
void Print_Process(process arr[]) {
    printf("\n[Process List]\n");
    printf("PID\tAT\tBT\tPR\n");
    for (int i = 0; i < process_num; i++) {
        printf("P%-2d\t%d\t%d\t%d\n", arr[i].pid, arr[i].arrival_time, arr[i].burst_time, arr[i].priority);
    }
}


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
            ori[i].arrival_time = rand() % MAX_ARRIVAL;      // 0 ~ 14
            ori[i].burst_time   = (rand() % MAX_BURST) + 1;   // 1 ~ 15
            ori[i].priority     = (rand() % process_num) + 1;// 1 ~ process_num
            p[i] = ori[i]; // 복사본 생성
        }
    } else {
        // 수동 입력
        for (int i = 0; i < process_num; i++) {
            ori[i].pid = i + 1;
            printf("Enter arrival time, CPU burst time, priority for [process %d]: ", ori[i].pid);
            scanf("%d %d %d", &ori[i].arrival_time, &ori[i].burst_time, &ori[i].priority);
            p[i] = ori[i];
        }
    }

    // 생성 결과 출력
    Print_Process(ori);
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
    
    if(no_print) return; // no_print가 1이면 evaluation 중이니 간트차트 스킵
    
    
    if (idx == -1) {
        // IDLE 구간
        printf("| %2d - %2d : IDLE\n", start, end);
    } else {
        // 프로세스 실행 구간
        printf("| %2d - %2d : P%d\n", start, end, idx);
    }
}

void FCFS(Queue *readyQ, Queue *waitQ) {
    int finished = 0; // 처리된 프로세스 개수
    int time = 0; // 현재 시간
    waiting_time[0] = 0;
    turnaround_time[0] = 0;

    // 초기화
    copy_process(); // ori[] -> p[] 복사
    Config(readyQ, waitQ); // 큐 초기화

    printf("\n--- FCFS Scheduling ---\n");

    Print_Process(p);
    
    printf("\n\n ----------------\n");
    
    // 시간 남으면 flag를 사용해서 enqueue 됐다? 그러면 패스 이런 식으로 enqueue 다 한 다음에 dequeue 루프 도는 방식 해보자.
    while (finished < process_num) {
        // 다음 실행할 프로세스 찾기
        int idx = -1; // 일단 후보가 없어.
        // * 만약에 arrival time 같으면 PID 순서로 enqueue 되게끔 수정
        for (int i = 0; i < process_num; i++) {
            if (p[i].burst_time == 0) continue; // 이미 처리된 프로세스
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
        waiting_time[0]    += (time - cur.arrival_time); // timeㄴ
        turnaround_time[0] += (time + cur.burst_time - cur.arrival_time);

        // 완료 되면 bt 만큼 time 옮겨주고 remaining 0으로, 처리된 거 하나 추가.
        time += cur.burst_time;
        p[idx].burst_time = 0;
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
    
    Print_Process(p);
    
    printf("\n\n ----------------\n");
    

    while (finished < process_num) {
        int idx = -1; // 우선 X

        // time 시점에 도착한 프로세스 중에서 bt가 가장 짧은 프로세스 찾아
        for (int i = 0; i < process_num; i++) {
            if (p[i].burst_time == 0) continue; //
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
                if (p[i].burst_time == 0) continue;
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
        p[idx].burst_time = 0;
        finished++;
    }
}

void Priority(Queue *readyQ, Queue *waitQ) {
    int time = 0;  // 현재 시간
    int finished = 0;  // 완료된 프로세스 수

    waiting_time[2] = 0;
    turnaround_time[2] = 0;

    // 초기화
    copy_process();
    Config(readyQ, waitQ);

    printf("\n--- Priority Scheduling ---\n");
    
    Print_Process(p);
    
    printf("\n\n ----------------\n");

    while (finished < process_num) {
        int idx = -1;

        // time 시점에 도착한 애들 중 우선순위가 가장 높은 놈 찾기
        for (int i = 0; i < process_num; i++) {
            if (p[i].burst_time == 0) continue;
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
                if (p[i].burst_time == 0) continue;
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
        p[idx].burst_time = 0;
        finished++;
    }
}

void RR(Queue *readyQ, Queue *waitQ) {
    int quantum = 3;
    int time = 0;
    int finished = 0;

    waiting_time[3] = 0;
    turnaround_time[3] = 0;

    
    copy_process();
    Config(readyQ, waitQ);

    printf("\n--- Round Robin Scheduling ---\n");
    
    Print_Process(p);
    
    printf("\n\n ----------------\n");

    while (finished < process_num) {
        int idx = -1;
        int end;

        // (1) 가장 먼저 도착한 프로세스를 스캔해서 idx 결정
        for (int i = 0; i < process_num; i++) {
            if (p[i].burst_time == 0) continue;
            if (idx == -1 || p[i].arrival_time < p[idx].arrival_time) {
                idx = i;
            }
        }

        // ready queue 거치는 과정
        Enqueue(readyQ, p[idx]);
        process cur = Dequeue(readyQ);

        // 도착 전이면 idle
        if (cur.arrival_time > time) {
            print_Gantt(time, cur.arrival_time, -1);
            time = cur.arrival_time;
        }

        // 프로세스 다 실행하느냐 아니면 time quantum 만큼만?
        end = time;
        waiting_time[3] += (time - cur.arrival_time);

        if (cur.burst_time <= quantum) {
            // remaining bt가 quantum 이하 → 완료
            turnaround_time[3] += (time + cur.burst_time - cur.arrival_time );
            end += cur.burst_time; // burst time 갱신
            p[idx].burst_time = 0;
            finished++;
        } else {
            // quantum 만큼만 실행 → remaining, arrival 갱신 (논리적으로 큐 맨 뒤로 보내는..)
            turnaround_time[3] += (time + quantum - cur.arrival_time );
            end += quantum;
            p[idx].burst_time -= quantum;
            p[idx].arrival_time = end;
        }

        // 간트 차트
        print_Gantt(time, end, cur.pid);

        // 현재 시점을 끝나는 시점으로
        time = end;
    }
}

int find_shortest(int time, int cur){
    int idx = -1;
    for (int i = 0; i < process_num; i++) {
        // 건너뛸 조건: 자기 자신이거나, 이미 완료되었거나, 아직 도착 전
        if (i == cur || p[i].burst_time == 0 || p[i].arrival_time > time)
            continue;

        if (idx != -1) {
            // 더 짧은 remaining, 같으면 더 빠른 arrival
            if (p[i].burst_time < p[idx].burst_time
             || (p[i].burst_time == p[idx].burst_time
                 && p[i].arrival_time < p[idx].arrival_time))
            {
                idx = i;
            }
        }
        else if (cur != -1) {
            // 첫 후보가 없고, cur이 실행 중이면 cur과 비교
            if (p[i].burst_time < p[cur].burst_time)
                idx = i;
        }
        else {
            // 후보도 cur도 없으면 바로 i를 후보로
            idx = i;
        }
    }
    return idx;
}

void preemptive_SJF(Queue *readyQ, Queue *waitQ) {
    int time = 0;
    int finished = 0;
    int cur = -1;  // 현재 CPU 점유 프로세스
    int cur_started = 0;  // cur이 점유 시작한 시각

    waiting_time[4] = 0;
    turnaround_time[4] = 0;

    // 초기화
    copy_process();
    Config(readyQ, waitQ);

    printf("\n--- Preemptive SJF ---\n");
    
    Print_Process(p);
    
    printf("\n\n ----------------\n");

    while (finished < process_num) {
        // time 시간에 도착한 프로세스들 중에서, cur보다 CPU burst가 짧은 프로세스의 인덱스를 리턴
        int next = find_shortest(time, cur);

        if (next != -1) { //-1이 아니라면 그런 프로세스가 존재
            // 만약 현재 IDLE 상태였다면, cur_started부터 time까지 idle했다고 출력
            if (cur == -1) {
                if (cur_started != time)
                    print_Gantt(cur_started, time, -1);
            }
            // 점유하던 프로세스가 있었다면, cur_started부터 time까지 점유했다고 출력
            else {
                print_Gantt(cur_started, time, p[cur].pid);
                // 다시 readyQ로 돌아갈 프로세스의 arrival_time 갱신
                p[cur].arrival_time = time;
            }

            // enqueue → dequeue
            Enqueue(readyQ, p[next]);
            process qcur = Dequeue(readyQ);
            int newCur = qcur.pid - 1;  // pid → 배열 인덱스

            // wt, tat 갱신
            waiting_time[4] += (time - p[newCur].arrival_time);
            turnaround_time[4] += (time - p[newCur].arrival_time);

            // 새 프로세스 점유
            cur = newCur;
            cur_started = time;
        }

        // 시간 갱신
        time++;

        // 실행 중인 프로세스가 있으면 remaining 감소·턴어라운드 증가
        if (cur != -1) {
            p[cur].burst_time--;
            turnaround_time[4]++;

            // 작업 완료 시
            if (p[cur].burst_time == 0) {
                print_Gantt(cur_started, time, p[cur].pid);
                finished++;
                // CPU idle 상태로
                cur = -1;
                cur_started = time;
            }
        }
    }
}

int find_highest(int time, int cur) {
    int idx = -1;
    for (int i = 0; i < process_num; i++) {
        // cur과 완료된 프로세스, 아직 도착 안 한 프로세스는 건너뜀
        if (i == cur || p[i].burst_time == 0 || p[i].arrival_time > time)
            continue;

        if (idx != -1) {
            // 우선순위가 높거나, 같으면 더 빠른 도착 시간
            if (p[i].priority > p[idx].priority
             || (p[i].priority == p[idx].priority && p[i].arrival_time < p[idx].arrival_time))
            {
                idx = i;
            }
        }
        else if (cur != -1) {
            // 첫 후보가 없고 cur이 실행 중이면 cur과 비교
            if (p[i].priority > p[cur].priority)
                idx = i;
        }
        else {
            // 후보도 cur도 없으면 바로 i를 후보로
            idx = i;
        }
    }
    return idx;
}

void preemptive_Priority(Queue *readyQ, Queue *waitQ) {
    int time = 0;
    int finished = 0;
    int cur = -1;   // 현재 점유 프로세스 인덱스
    int cur_started = 0;    // cur 점유 시작 시각

    waiting_time[5] = 0;
    turnaround_time[5] = 0;

    // 초기화
    copy_process();
    Config(readyQ, waitQ);

    printf("\n--- Preemptive Priority Scheduling ---\n");
    
    Print_Process(p);
    
    printf("\n\n ----------------\n");

    while (finished < process_num) {
        // 다음 실행할 프로세스 선택
        int next = find_highest(time, cur);

        if (next != -1) {
            //IDLE 상태에서 프로세스 도착 전 구간
            if (cur == -1) {
                if (cur_started != time)
                    print_Gantt(cur_started, time, -1);
            }
            //실행 중이던 프로세스 구간
            else {
                print_Gantt(cur_started, time, p[cur].pid);
                // 선점된 프로세스는 다시 대기 큐로 돌아가므로 도착 시간 갱신
                p[cur].arrival_time = time;
            }

            //enqueue → dequeue
            Enqueue(readyQ, p[next]);
            process dispatched = Dequeue(readyQ);
            int newCur = dispatched.pid - 1;

            //wt, tat 갱신
            waiting_time[5] += (time - p[newCur].arrival_time);
            turnaround_time[5] += (time - p[newCur].arrival_time);

            // 새 프로세스 시작
            cur = newCur;
            cur_started = time;
        }

        //시간 증가
        time++;

        // 실행 중인 프로세스가 있으면 remaining 감소·턴어라운드 증가
        if (cur != -1) {
            p[cur].burst_time--;
            turnaround_time[5]++;

            // 작업 완료 시
            if (p[cur].burst_time == 0) {
                print_Gantt(cur_started, time, p[cur].pid);
                finished++;
                cur = -1;
                cur_started = time;
            }
        }
    }
}

void Schedule(void){
    int choice;

    while(1){
        printf("\nwhich scheduling?\n");

        for(int i = 0; i < SCHEDUER + 1; i++)
            printf("%d. %s / ", i+1, match[i]);
        
        scanf("%d", &choice);

        if(choice == SCHEDUER + 1) break;



        printf("\n---%s start---\n", match[choice-1]);

        Queue readyQ, waitQ;
        
        switch (choice)
        {
        case 1:
            FCFS(&readyQ, &waitQ);
            break;
        case 2:
            SJF(&readyQ, &waitQ);
            break;
        case 3:
            Priority(&readyQ, &waitQ);
            break;
        case 4:
            RR(&readyQ, &waitQ);
            break;
        case 5:
            preemptive_SJF(&readyQ, &waitQ);
            break;
        case 6:
            preemptive_Priority(&readyQ, &waitQ);
            break;
        default:
            break;
        }

        printf("---%s end---\n", match[choice-1]);
    }
}

void Evaluation(void){
    int min_wait_idx = -1, min_turn_idx = -1; // 각각 waiting time과 turnaround time이 최소인 스케줄링의 인덱스
    printf("\n---Average waiting time and turnaround time in each algorithm---\n");

    for(int i = 0; i < SCHEDUER; i++){
        float avg_wait, avg_turn;

        if(turnaround_time[i] == 0) continue;

        avg_wait = (float)waiting_time[i]/process_num;
        avg_turn = (float)turnaround_time[i]/process_num;

        printf("%d. %s: %.2f, %.2f\n", i+1, match[i], avg_wait, avg_turn);
        
        if(min_wait_idx == -1 || waiting_time[i] < waiting_time[min_wait_idx]) min_wait_idx = i;
        if(min_turn_idx == -1 || turnaround_time[i] < turnaround_time[min_turn_idx]) min_turn_idx = i;
    }

    if(min_wait_idx != -1){
        printf("\nAlgorithm with minimum waiting time is %s\n", match[min_wait_idx]);
        printf("Algorithm with minimum turnaround time is %s\n", match[min_turn_idx]);
    }
    else printf("No data\n");
}


int main(void) {
    
        Creat_Process();
    
        Schedule();

        Evaluation();

    return 0;
}

