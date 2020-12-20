# migrate system
a migrate system for cluster.

## how to use?
1. make: build and link as binary file main in ./bin
2. make clean: clean *.o main and bak file in ./bin/file/

3. make tb: test base performance (random read, all content is in memory)
4. make tt: test performance with tossd thread(randon read, 40% in ssd and 60% in memory)
5. make tm: test performance with migrate thread(randon read, 40% in remote memory and 60% in local memory)
6. make tc: a client pull data from machine who run "make tm" and wait for pagefault_fault evicted


## directory list
```
tree -L 2
.
├── bin
│   ├── bak.sh          //bak file, add path to crontab
│   ├── file            //folder for to_ssd
│   ├── ips.cfg         //configure file, ip,T1,T2...
│   ├── log             //folder for log
│   ├── main            //main binary file
│   ├── perf            //perf binary file
│   ├── r.sh            //base test, server run
│   └── t.sh            //base test, client run
├── common
│   ├── list.c          //list module
│   ├── log.c           //log module
│   ├── rbtree.c        //rbtree module
│   └── tools.c         //tools module
├── include
│   ├── common.h        //common header, for rdma ib_verbs
│   ├── ctl.h           //control header, for compiler
│   ├── list.h          //list header
│   ├── log.h           //log header
│   ├── message.h       //message header, for rdma connect
│   ├── migcomm.h       //migrate common header
│   ├── migrate.h       //migrate header
│   ├── perf_comm.h     //common header for perf
│   ├── rbtree.h        //rbtree header
│   ├── rdma.h          //rdma header
│   ├── rpc.h           //rpc header
│   ├── throughput.h    //throught test header
│   ├── tools.h         //tools header
│   └── userfaultfd.h   //userfaultfd header
├── main.c              //main function, just run this file in production environment
├── Makefile            //makefile, compile event and some command
├── migrate
│   ├── migcomm.c       //common function for migration
│   ├── migrate.c       //function for migration
│   ├── rpc.c           //function for rpc
│   └── userfaultfd.c   //function for userfaultfd
├── perf
│   ├── perf_comm.c     //common function for performance test
│   └── throughput.c    //throughput test
├── perf.c              //perf file, run this file in development environment to test...
├── rdma
│   ├── common.c        //common function for rdma connection
│   └── rdma.c          //rdma implement with ib_verbs
├── README.md
├── test
│   ├── log             //log test folder
│   ├── malloc          //malloc test folder
│   ├── rbtree          //rbtree test folder
│   ├── rdma            //rdma test folder
│   ├── rpc             //rpc test folder
│   └── time            //time test folder
└── w.sh                //just a shell script
```

## time line

| time  | author | content |
| ----  | -----  | ------  |
| 2020.10.09 | abin  | combine and fix some erroes, throughput function is ok! |
| 2020.10.12 | abin  | print errno info, need to debug! |
