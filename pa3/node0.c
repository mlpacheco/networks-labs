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

int connectcosts0[4] = {0, 1, 3, 7};
int neighbors0[4] = {0, 1, 1, 1};
int x0 = 0;

struct distance_table
{
  int costs[4][4];
} dt0;


/* students to write the following two routines, and maybe some others */

void senddistvect0() {
    printf("ROUTER %d: Sending distance vectors to neighbors...\n", x0);
    int w;
    for (w = 0; w < 4; w++) {
        if (neighbors0[w] == 0)
            continue;

        struct rtpkt *mypktptr;
        mypktptr = (struct rtpkt *)malloc(sizeof(struct rtpkt));

        mypktptr->sourceid = x0;
        mypktptr->destid = w;

        int y;
        for (y = 0; y < 4; y++) {
            mypktptr->mincost[y] = dt0.costs[y][x0];
        }

        printf("\tROUTER %d, Sending distance vector to neighbour %d: [%d %d %d %d]\n", x0, w,
            dt0.costs[0][x0], dt0.costs[1][x0], dt0.costs[2][x0], dt0.costs[3][x0]);
        tolayer2(*mypktptr);
    }
}

void rtinit0()
{
    // initialize routing table with neighbors direct costs
    printf("ROUTER %d: Initializing routing table...\n", x0);
    int w, i, j;

    for (i = 0; i < 4; i++) {
        for (j = 0; j < 4; j++) {
            dt0.costs[i][j] = 999;
        }
    }

    for (w = 0; w < 4; w++) {
        dt0.costs[w][x0] = connectcosts0[w];
    }

    // pretty print distance table
    printdt0(&dt0);

    // for each neighbor, send its min cost path
    senddistvect0();
}

int bellmanfordupdt0() {
    // execute the bellman-ford updates for all routers
    printf("ROUTER %d: Executing Bellman-Ford updates\n", x0);
    int y, v;
    int changed = 0;

    for (y = 0; y < 4; y++) {
        int min_v = connectcosts0[y];

        for (v = 0; v < 4; v++) {
            // skip non-neighbors
            if (neighbors0[v] == 0)
                continue;

            if (connectcosts0[v] + dt0.costs[y][v] < min_v) {
                min_v = connectcosts0[v] + dt0.costs[y][v];
            }
        }

        if (min_v != dt0.costs[y][x0]) {
            changed = 1;
            dt0.costs[y][x0] = min_v;
        }
    }

    return changed;

}

void rtupdate0(rcvdpkt)
  struct rtpkt *rcvdpkt;
{
    int w = rcvdpkt->sourceid;
    int y;

    // update distance vector of neighbor w in distance table
    printf("ROUTER %d: Updating neighbor %d distance vector\n", x0, w);
    for (y = 0; y < 4; y++) {
        dt0.costs[y][w] = rcvdpkt->mincost[y];
    }

    int changed = bellmanfordupdt0();

    if (changed) {
        printf("ROUTER %d: My distance vector has changed\n", x0);
        senddistvect0();
    }

    printdt0(&dt0);
}


printdt0(dtptr)
  struct distance_table *dtptr;
{
  printf("                via     \n");
  printf("   D0 |    1     2    3 \n");
  printf("  ----|-----------------\n");
  printf("     1|  %3d   %3d   %3d\n",dtptr->costs[1][1],
	 dtptr->costs[1][2],dtptr->costs[1][3]);
  printf("dest 2|  %3d   %3d   %3d\n",dtptr->costs[2][1],
	 dtptr->costs[2][2],dtptr->costs[2][3]);
  printf("     3|  %3d   %3d   %3d\n",dtptr->costs[3][1],
	 dtptr->costs[3][2],dtptr->costs[3][3]);
}

/* called when cost from 0 to linkid changes from current value to newcost*/
/* You can leave this routine empty if you're an undergrad. If you want */
/* to use this routine, you'll need to change the value of the LINKCHANGE */
/* constant definition in prog3.c from 0 to 1 */
void linkhandler0(linkid, newcost)
  int linkid, newcost;
{
    int prevcost = connectcosts0[linkid];
    printf("\nROUTER %d: cost has changed for link %d from %d to %d\n", x0, linkid, prevcost, newcost);

    connectcosts0[linkid] = newcost;
    int changed = bellmanfordupdt0();

    if (changed) {
        senddistvect0();
    }

    printdt0(&dt0);
}

