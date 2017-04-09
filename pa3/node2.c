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

int connectcosts2[4] = {3,  1,  0, 2};
int neighbors2[4] = {1, 1, 0, 1};
int x2 = 2;

struct distance_table
{
  int costs[4][4];
} dt2;


/* students to write the following two routines, and maybe some others */

void senddistvect2() {
    printf("ROUTER %d: Sending distance vectors to neighbors...\n", x2);
    int w;
    for (w = 0; w < 4; w++) {
        if (neighbors2[w] == 0)
            continue;

        struct rtpkt *mypktptr;
        mypktptr = (struct rtpkt *)malloc(sizeof(struct rtpkt));

        mypktptr->sourceid = x2;
        mypktptr->destid = w;

        int y;
        for (y = 0; y < 4; y++) {
            mypktptr->mincost[y] = dt2.costs[y][x2];
        }

        printf("\tROUTER %d, Sending distance vector to neighbour %d: [%d %d %d %d]\n", x2, w,
            dt2.costs[0][x2], dt2.costs[1][x2], dt2.costs[2][x2], dt2.costs[3][x2]);
        tolayer2(*mypktptr);
    }
}

void rtinit2()
{
    // initialize routing table with neighbors direct costs
    printf("ROUTER %d: Initializing routing table...\n", x2);
    int w, i, j;

    for (i = 0; i < 4; i++) {
        for (j = 0; j < 4; j++) {
            dt2.costs[i][j] = 999;
        }
    }

    for (w = 0; w < 4; w++) {
        dt2.costs[w][x2] = connectcosts2[w];
    }

    // pretty print distance table
    printdt2(&dt2);

    // for each neighbor, send its min cost path
    senddistvect2();
}


void rtupdate2(rcvdpkt)
  struct rtpkt *rcvdpkt;
{
    int w = rcvdpkt->sourceid;
    int y, v;

    // update distance vector of neighbor w in distance table
    printf("ROUTER %d: Updating neighbor %d distance vector\n", x2, w);
    for (y = 0; y < 4; y++) {
        dt2.costs[y][w] = rcvdpkt->mincost[y];
    }

    // execute the bellman-ford updates for all routers
    printf("ROUTER %d: Executing Bellman-Ford updates\n", x2);
    int changed = 0;
    for (y = 0; y < 4; y++) {
        int min_v = connectcosts2[y];

        for (v = 0; v < 4; v++) {
            // skip non-neighbors
            if (neighbors2[v] == 0)
                continue;

            if (connectcosts2[v] + dt2.costs[y][v] < min_v) {
                min_v = connectcosts2[v] + dt2.costs[y][v];
            }
        }

        if (min_v != dt2.costs[y][x2]) {
            changed = 1;
            dt2.costs[y][x2] = min_v;
        }
    }

    if (changed) {
        printf("ROUTER %d: My distance vector changed\n", x2);
        senddistvect2();
    }

    printdt2(&dt2);

}


printdt2(dtptr)
  struct distance_table *dtptr;
{
  printf("                via     \n");
  printf("   D2 |    0     1    3 \n");
  printf("  ----|-----------------\n");
  printf("     0|  %3d   %3d   %3d\n",dtptr->costs[0][0],
	 dtptr->costs[0][1],dtptr->costs[0][3]);
  printf("dest 1|  %3d   %3d   %3d\n",dtptr->costs[1][0],
	 dtptr->costs[1][1],dtptr->costs[1][3]);
  printf("     3|  %3d   %3d   %3d\n",dtptr->costs[3][0],
	 dtptr->costs[3][1],dtptr->costs[3][3]);
}
