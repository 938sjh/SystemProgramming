<h2>System Programming Project</h2>
2022-Spring Sogang Univ. System Programming(CSE4100) 
<h2>Project1. Linux MyShell</h2>
Linux MyShell 구현

-fork & signal

+ cd, ls, mkdir 등과 같은 기본적인 shell command 구현
+ ctrl + c 를 통해 현재 foreground에서 실행되고 있는 process 종료 구현

-pipelining

+ 기본적인 shell command와 더불어 pipeline을 이용해 process간 communication 구현
+ ls -al | grep abc에서와 같이 ls -al의 결과를 바탕으로 grep 실행 구현

-background process

+ command들을 background로 실행시킬 수 있도록 함
+ fg, bg, jobs, kill이라는 builtin command를 추가로 구현
+ 현재 running, stopped 상태인 background job을 확인하는 등 job과 관련된 command 구현


<h2>Project2. Concurrent Stock Server</h2>

여러 명의 client가 동시에 접속하여 Concurrent하게 서비스할 수 있는 Concurrent 주식 서버를 구현

-Event-driven Approach

+ Select함수를 이용해 어떤 descriptor에 pending input이 있는지 확인
  
+ pending input이 있으면 이를 event라고 함
  
+ 이를 이용하여 여러 명의 client가 주식 서버에 connection을 요청할 수 있도록 구현
  
+ 주식 서버에서 구매, 판매, exit, 목록 조회 구현
  
+ Select를 이용하여 pending input이 있는 file descriptor들만 작업을 수행해주어 여러 client를 concurrent하게 서비스할 수 있도록 함

-Thread-based Approach

+ Thread를 이용하여 Event-based Server처럼 여러 명의 client가 주식 서버에 connection 요청할 수 있도록 구현
  
+ thread를 미리 여러개 생성해두고 client로부터 요청이 오면 connection 요청을 수락해 미리 생성한 thread를 이용하여 client의 작업을 처리해주어 여러 client를 concurrent하게 서비스할 수 있도록 함
  
-Performance Evaluation

+ client 수가 많아짐에 따라 thread-based가 event-driven에 비해 더 효율적이고 show만 처리할 때에는 thread-based가 더 좋은 성능을 buy, sell만 할 때에는 event-driven이 더 효율적

<h2>Project3. Memory Allocation</h2>

c언어 library의 malloc, free, realloc과 유사 한 성능을 내는 mm_malloc, mm_free, mm_realloc을 구현

-mm_init

+ implicit free list에서의 init과 동일
 
+ Initial heap을 만든 후 prologue header, prologue footer, epilogue header에 대한 공간을 할당

-mm_malloc

+ 기존의 implicit free list와 유사
  
+ find_fit 함수에서의 차이
  
+ Implicit free list에서는 모든 블록을 추적하는 것이었다면 explicit free list에서는 free인 블록만 살핌

-mm_free

+ mm_free에서의 coalesce함수는 Implicit free list에서의 coalesce와 동일하게 case1~4를 앞, 뒤 블록이 free인지의 여부에 따라 나눈 후 그에 맞게 free 블록들을 join 함

-mm_realloc

+ 원하는 사이즈에서 HEADER와 FOOTER에 해당하는 추가 8바이트를 size에 더 추가
  
+ 만약 뒤에 블록이 free이고 현재 블록과 뒤에 블록을 합쳤을 때 길이가 위에서 구한 size+8보다 작거나 같으면 현재 위치에서 realloc
  
+ 그렇지 않으면 새로운 size에 맞는 곳을 찾아서 malloc해주고 기존의 block은 free
