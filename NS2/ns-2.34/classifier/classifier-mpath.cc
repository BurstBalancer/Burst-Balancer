/* -*- Mode:C++; c-basic-offset:8; tab-width:8; indent-tabs-mode:t
              -*- */

/*
 * Copyright (C) 1997 by the University of Southern California
 * $Id: classifier-mpath.cc,v 1.10 2005/08/25 18:58:01 johnh Exp $
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License,
 * version 2, as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 59 Temple Place, Suite 330, Boston, MA 02111-1307, USA.
 *
 *
 * The copyright of this module includes the following
 * linking-with-specific-other-licenses addition:
 *
 * In addition, as a special exception, the copyright holders of
 * this module give you permission to combine (via static or
 * dynamic linking) this module with free software programs or
 * libraries that are released under the GNU LGPL and with code
 * included in the standard release of ns-2 under the Apache 2.0
 * license or under otherwise-compatible licenses with advertising
 * requirements (or modified versions of such code, with unchanged
 * license).  You may copy and distribute such a system following the
 * terms of the GNU GPL for this module and the licenses of the
 * other code concerned, provided that you include the source code of
 * that other code when and as the GNU GPL requires distribution of
 * source code.
 *
 * Note that people who make modified versions of this module
 * are not obligated to grant this special exception for their
 * modified versions; it is their choice whether to do so.  The GNU
 * General Public License gives permission to release a modified
 * version without this exception; this exception also makes it
 * possible to release a modified version which carries forward this
 * exception.
 *
 */

#ifndef lint
static const char rcsid[] =
    "@(#) $Header: /cvsroot/nsnam/ns-2/classifier/classifier-mpath.cc,v 1.10 "
    "2005/08/25 18:58:01 johnh Exp $ (USC/ISI)";
#endif

#include "classifier.h"
#include "connector.h"
#include "ip.h"
#include "murmur3.h"
#include "priority.h"
#include "scheduler.h"
#include "tcp.h"
#include <climits>
#include <iostream>

class MultiPathForwarder : public Classifier {
public:
  MultiPathForwarder()
      : ns_(0), nodeid_(0), nodetype_(0), perflow_(0), checkpathid_(0),
        balanceAlg_(0), delta_flow_(1e-3), delta_flet_(1e-4), seed_(1806) {
    bind("nodeid_", &nodeid_);
    bind("nodetype_", &nodetype_);
    bind("perflow_", &perflow_);
    bind("checkpathid_", &checkpathid_);

    bind("balanceAlg_", &balanceAlg_);
    bind("delta_flow_", &delta_flow_);
    bind("delta_flet_", &delta_flet_);

    bind("seed_", &seed_);

    nb_packet = NULL;
  }

  virtual int classify(Packet *p) {
    hdr_ip *h = hdr_ip::access(p);
    double ts = Scheduler::instance().clock();
    struct hkey {
      int nodeid;
      nsaddr_t src, dst;
      int fid;
    };
    struct hkey buf_;
    buf_.nodeid = nodeid_;
    buf_.src = mshift(h->saddr());
    buf_.dst = mshift(h->daddr());
    buf_.fid = h->flowid();

    char *bufString = (char *)&buf_;
    int length = sizeof(hkey);

    if (nb_packet == NULL) {
      nb_packet = (int *)malloc(sizeof(int) * maxslot_);
      memset(nb_packet, 0, sizeof(int) * maxslot_);
    }

    // fprintf(stdout, "packet_info: (%p, %d: %p, %p) [%d %d %d] (%d)\n", this,
    //         buf_.nodeid, B, D, buf_.src, buf_.dst, buf_.fid, maxslot_);

    unsigned int fid = (unsigned int)HashString(bufString, length, seed_);

    // int nb_slot = 0;
    // for (int i = 0; i < maxslot_ + 1; i++) {
    //   if (slot_[i] != 0) {
    //     nb_slot += 1;
    //   }
    // }
    // if (nb_slot > 1) {
    //   std::cout << "nodeid: " << nodeid_ << " - ";
    //   for (int i = 0; i < maxslot_ + 1; i++) {
    //     if (slot_[i] != 0) {
    //       std::cout << slot_[i] << " ";
    //     }
    //   }
    //   std::cout << std::endl;
    // }

    if (balanceAlg_ == 0) {
      /*
       * ECMP
       */
      return lookup(fid, h->prio());
    } else if (balanceAlg_ == 1) {
      /*
       * BB
       */
      unsigned hash = Hash(fid, 0, seed_);

      flowburst &u = B[hash % memsize_];
      if (!u.valid || ts - u.time > delta_flow_) {
        u = flowburst(fid, 1, -1, ts);
        int cl = lookup(fid, h->prio());

        return cl;
      } else if (u.id != fid) {
        if (u.freq > 0) {
          u.freq--;
        } else if (u.egress == -1) {
          u = flowburst(fid, 1, -1, ts);
        }

        return lookup(fid, h->prio());
      } else {
        if (ts - u.time > delta_flet_) {
          // u.egress = 0;
          // int min_load = INT_MAX;
          // for (int i = 0; i < maxslot_ + 1; i++) {
          //   if (slot_[i] != 0 && nb_packet[i] < min_load) {
          //     u.egress = i;
          //     min_load = nb_packet[i];
          //   }
          // }
          if (u.egress == -1) {
            u.egress = fid;
          } else {
            u.egress = Hash(u.egress, 0, seed_);
          }
        }

        u.freq++;
        u.time = ts;

        if (u.egress == -1) {
          return lookup(fid, h->prio());
        } else {
          return lookup(u.egress, h->prio());
        }
      }
    } else if (balanceAlg_ == 2) {
      /*
       * LetFlow
       */
      unsigned hash = Hash(fid, 0, seed_);
      flowlet &u = D[hash % memsize_];
      if (!u.valid) {
        u = flowlet(fid, ts);

        // fprintf(stdout,
        //         "new init at node %d: src %d dst %d fid %d port %d"
        //         "%u\n",
        //         buf_.nodeid, buf_.src, buf_.dst, buf_.fid,
        //         (uint32_t)(u.egress) % (maxslot_ + 1));
      } else if (ts - u.time > delta_flet_) {
        // u.egress = 0;
        // int min_load = INT_MAX;
        // for (int i = 0; i < maxslot_ + 1; i++) {
        //   if (slot_[i] != 0 && nb_packet[i] < min_load) {
        //     u.egress = i;
        //     min_load = nb_packet[i];
        //   }
        // }

        // fprintf(stdout, "new flowlet at node %d: src %d dst %d fid %d\n",
        //         buf_.nodeid, buf_.src, buf_.dst, buf_.fid);
        u.egress = Hash(u.egress, 0, seed_);
      }

      u.time = ts;
      return lookup(u.egress, h->prio());
    } else if (balanceAlg_ == 3) {
      /*
       * DRILL
       */
      uint32_t rand_choice_1 = Hash(ts, 1, 1, seed_) % (maxslot_ + 1);
      uint32_t rand_choice_2 = Hash(ts, 2, 2, seed_) % (maxslot_ + 1);

      Priority *queue_0 =
          (Priority *)(((Connector *)slot_[*last_choice_])->target());
      Priority *queue_1 =
          (Priority *)(((Connector *)slot_[rand_choice_1])->target());
      Priority *queue_2 =
          (Priority *)(((Connector *)slot_[rand_choice_2])->target());

      uint32_t length_0 = queue_0->TotalByteLength();
      uint32_t length_1 = queue_1->TotalByteLength();
      uint32_t length_2 = queue_2->TotalByteLength();

      // fprintf(stderr, "drill:%p:%d %u:%p:%u %u:%p:%u %u:%p:%u\n", this,
      // nodeid_,
      //         *last_choice_, queue_0, length_0, rand_choice_1, queue_1,
      //         length_1, rand_choice_2, queue_2, length_2);

      if (length_0 <= length_1 && length_0 <= length_2) {
        *last_choice_ = *last_choice_;
      } else if (length_1 <= length_0 && length_1 <= length_2) {
        *last_choice_ = rand_choice_1;
      } else if (length_2 <= length_0 && length_2 <= length_1) {
        *last_choice_ = rand_choice_2;
      }
      // fprintf(stderr, "drill:%p:%d last_choice_:%d dst_:%d\n", this, nodeid_,
      //         *last_choice_, buf_.dst);
      return lookup(*last_choice_, h->prio());
    }
  }

  virtual int lookup(unsigned int ms_, int prio) {
    int cl;
    ms_ %= (maxslot_ + 1);
    int fail = ms_;
    do {
      cl = ms_++;
      ms_ %= (maxslot_ + 1);
    } while (slot_[cl] == 0 && ms_ != fail);

    nb_packet[cl] += 1;

    return cl;
  }

private:
  int ns_;
  // Mohamamd: adding support for perflow multipath
  int nodeid_;
  int nodetype_;
  int perflow_;
  int checkpathid_;
  int balanceAlg_;
  uint32_t seed_;

  double delta_flow_;
  double delta_flet_;

  int *nb_packet;

  static unsigned int Hash(unsigned int x, unsigned int y, uint32_t seed) {
    unsigned int arr[] = {x, y};
    return HashString((char *)arr, 8, seed);
  }

  static unsigned int Hash(double x, double y, double z, uint32_t seed) {
    double arr[] = {x, y, z};
    return HashString((char *)arr, 16, seed);
  }

  static unsigned int HashString(register const char *bytes, int length,
                                 uint32_t seed) {
    register unsigned int result;
    register int i;
    result = 0;
    // for (i = 0; i < length; i++) {
    //   result += (result << 3) + *bytes++;
    // }
    //
    MurmurHash3_x86_32(bytes, length, seed, &result);

    return result;
  }

  // static const int MEMSIZE = 10;

  // struct flowburst {
  //   unsigned int id;
  //   int freq;
  //   int egress;
  //   double time;
  //   bool valid;
  //   flowburst() {
  //     id = freq = time = 0;
  //     valid = false;
  //     egress = -1;
  //   }
  //   flowburst(unsigned int id, int freq, int egress, double time)
  //       : id(id), freq(freq), egress(egress), time(time), valid(true) {}
  // } B[MEMSIZE];

  // struct flowlet {
  //   int egress;
  //   double time;
  //   bool valid;
  //   flowlet() {
  //     time = 0;
  //     valid = false;
  //     egress = -1;
  //   }
  //   flowlet(int egress, double time)
  //       : egress(egress), time(time), valid(true) {}
  // } D[MEMSIZE];
};

static class MultiPathClass : public TclClass {
public:
  MultiPathClass() : TclClass("Classifier/MultiPath") {}
  TclObject *create(int, const char *const *) {
    return (new MultiPathForwarder());
  }
} class_multipath;
