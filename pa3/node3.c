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

int connectcosts3[4] = {7, 999, 2, 0};
int neighbors3[4] = {1, 0, 1, 0};
int x3 = 3;

struct distance_table
{
  int costs[4][4];
} dt3;


/* students to write the following two routines, and maybe some others */

void senddistvect3() {
    printf("ROUTER %d: Sending distance vectors to neighbors...\n", x3);
    int w;
    for (w = 0; w < 4; w++) {
        if (neighbors3[w] == 0)
            continue;

        struct rtpkt *mypktptr;
        mypktptr = (struct rtpkt *)malloc(sizeof(struct rtpkt));

        mypktptr->sourceid = x3;
        mypktptr->destid = w;

        int y;
        for (y = 0; y < 4; y++) {
            mypktptr->mincost[y] = dt3.costs[y][x3];
        }

        printf("\tROUTER %d, Sending distance vector to neighbour %d: [%d %d %d %d]\n", x3, w,
            dt3.costs[0][x3], dt3.costs[1][x3], dt3.costs[2][x3], dt3.costs[3][x3]);
        tolayer2(*mypktptr);
    }
}

void rtinit3()
{
    // initialize routing table with neighbors direct costs
    printf("ROUTER %d: Initializing routing table...\n", x3);
    int w, i, j;

    for (i = 0; i < 4; i++) {
        for (j = 0; j < 4; j++) {
            dt3.costs[i][j] = 999;
        }
    }

    for (w = 0; w < 4; w++) {
        dt3.costs[w][x3] = connectcosts3[w];
    }

    // pretty print distance table
    printdt3(&dt3);

    // for each neighbor, send its min cost path
    senddistvect3();
}

void rtupdate3(rcvdpkt)
  struct rtpkt *rcvdpkt;
{
    int w = rcvdpkt->sourceid;
    int y, v;

    // update distance vector of neighbor w in distance table
    printf("ROUTER %d: Updating neighbor %d distance vector\n", x3, w);
    for (y = 0; y < 4; y++) {
        dt3.costs[y][w] = rcvdpkt->mincost[y];
    }

    // execute the bellman-ford updates for all routers
    printf("ROUTER %d: Executing Bellman-Ford updates\n", x3);
    int changed = 0;
    for (y = 0; y < 4; y++) {
        int min_v = dt3.costs[y][x3];

        for (v = 0; v < 4; v++) {
            // skip non-neighbors
            if (neighbors3[v] == 0)
                continue;

            if (dt3.costs[v][x3] + dt3.costs[y][v] < min_v) {
                min_v = dt3.costs[v][x3] + dt3.costs[y][v];
            }
        }

        if (min_v != dt3.costs[y][x3]) {
            changed = 1;
            dt3.costs[y][x3] = min_v;
        }
    }

    if (changed) {
        senddistvect3();
    }

    printdt3(&dt3);

}


printdt3(dtptr)
  struct distance_table *dtptr;
{
  printf("             via     \n");
  printf("   D3 |    0     2 \n");
  printf("  ----|-----------\n");
  printf("     0|  %3d   %3d\n",dtptr->costs[0][0], dtptr->costs[0][2]);
  printf("dest 1|  %3d   %3d\n",dtptr->costs[1][0], dtptr->costs[1][2]);
  printf("     2|  %3d   %3d\n",dtptr->costs[2][0], dtptr->costs[2][2]);

}
