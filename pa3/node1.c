#include <stdio.h>

extern struct rtpkt {
  int sourceid;       /* id of sending router sending this pkt */
  int destid;         /* id of router to which pkt being sent 
                         (must be an immediate neighbor) */
  int mincost[4];    /* min cost to node 0 ... 3 */
  };


extern int TRACE;
extern int YES;
extern int NO;

int connectcosts1[4] = {1,  0,  1, 999};
int neighbors1[4] = {1, 0, 1, 0};
int x1 = 1;

struct distance_table
{
  int costs[4][4];
} dt1;


/* students to write the following two routines, and maybe some others */

void senddistvect1() {
    printf("ROUTER %d: Sending distance vectors to neighbors...\n", x1);
    int w;
    for (w = 0; w < 4; w++) {
        if (neighbors1[w] == 0)
            continue;

        struct rtpkt *mypktptr;
        mypktptr = (struct rtpkt *)malloc(sizeof(struct rtpkt));

        mypktptr->sourceid = x1;
        mypktptr->destid = w;

        int y;
        for (y = 0; y < 4; y++) {
            mypktptr->mincost[y] = dt1.costs[y][x1];
        }

        printf("\tROUTER %d, Sending distance vector to neighbour %d: [%d %d %d %d]\n", x1, w,
            dt1.costs[0][x1], dt1.costs[1][x1], dt1.costs[2][x1], dt1.costs[3][x1]);
        tolayer2(*mypktptr);
    }
}

void rtinit1()
{
    // initialize routing table with neighbors direct costs
    printf("ROUTER %d: Initializing routing table...\n", x1);
    int w, i, j;

    for (i = 0; i < 4; i++) {
        for (j = 0; j < 4; j++) {
            dt1.costs[i][j] = 999;
        }
    }

    for (w = 0; w < 4; w++) {
        dt1.costs[w][x1] = connectcosts1[w];
    }

    // pretty print distance table
    printdt1(&dt1);

    // for each neighbor, send its min cost path
    senddistvect1();
}


void rtupdate1(rcvdpkt)
  struct rtpkt *rcvdpkt;

{
    int w = rcvdpkt->sourceid;
    int y, v;

    // update distance vector of neighbor w in distance table
    printf("ROUTER %d: Updating neighbor %d distance vector\n", x1, w);
    int changed = 0;
    for (y = 0; y < 4; y++) {
        dt1.costs[y][w] = rcvdpkt->mincost[y];
    }

    // execute the bellman-ford updates for all routers
    printf("ROUTER %d: Executing Bellman-Ford updates\n", x1);
    for (y = 0; y < 4; y++) {
        int min_v = dt1.costs[y][x1];

        for (v = 0; v < 4; v++) {
            // skip non-neighbors
            if (neighbors1[v] == 0)
                continue;

            if (dt1.costs[v][x1] + dt1.costs[y][v] < min_v) {
                min_v = dt1.costs[v][x1] + dt1.costs[y][v];
            }
        }

        if (min_v != dt1.costs[y][x1]) {
            changed = 1;
            dt1.costs[y][x1] = min_v;
        }

    }

    if (changed) {
        senddistvect1();
    }

    printdt1(&dt1);

}


printdt1(dtptr)
  struct distance_table *dtptr;
  
{
  printf("             via   \n");
  printf("   D1 |    0     2 \n");
  printf("  ----|-----------\n");
  printf("     0|  %3d   %3d\n",dtptr->costs[0][0], dtptr->costs[0][2]);
  printf("dest 2|  %3d   %3d\n",dtptr->costs[2][0], dtptr->costs[2][2]);
  printf("     3|  %3d   %3d\n",dtptr->costs[3][0], dtptr->costs[3][2]);

}



void linkhandler1(linkid, newcost)   
int linkid, newcost;   
/* called when cost from 1 to linkid changes from current value to newcost*/
/* You can leave this routine empty if you're an undergrad. If you want */
/* to use this routine, you'll need to change the value of the LINKCHANGE */
/* constant definition in prog3.c from 0 to 1 */
	
{
}


