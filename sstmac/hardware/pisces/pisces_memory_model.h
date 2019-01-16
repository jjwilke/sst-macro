/**
Copyright 2009-2018 National Technology and Engineering Solutions of Sandia, 
LLC (NTESS).  Under the terms of Contract DE-NA-0003525, the U.S.  Government 
retains certain rights in this software.

Sandia National Laboratories is a multimission laboratory managed and operated
by National Technology and Engineering Solutions of Sandia, LLC., a wholly 
owned subsidiary of Honeywell International, Inc., for the U.S. Department of 
Energy's National Nuclear Security Administration under contract DE-NA0003525.

Copyright (c) 2009-2018, NTESS

All rights reserved.

Redistribution and use in source and binary forms, with or without modification, 
are permitted provided that the following conditions are met:

    * Redistributions of source code must retain the above copyright
      notice, this list of conditions and the following disclaimer.

    * Redistributions in binary form must reproduce the above
      copyright notice, this list of conditions and the following
      disclaimer in the documentation and/or other materials provided
      with the distribution.

    * Neither the name of the copyright holder nor the names of its
      contributors may be used to endorse or promote products derived
      from this software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
"AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

Questions? Contact sst-macro-help@sandia.gov
*/

#ifndef pisces_MEMORY_MODEL_H
#define pisces_MEMORY_MODEL_H

#include <sstmac/hardware/memory/memory_model.h>
#include <sstmac/software/libraries/compute/compute_event_fwd.h>
#include <sstmac/hardware/pisces/pisces_arbitrator.h>
#include <sstmac/hardware/pisces/pisces_sender.h>
#include <sprockit/thread_safe_new.h>
#include <sprockit/allocator.h>

namespace sstmac {
namespace hw {

class PiscesMemoryModel : public MemoryModel
{
  FactoryRegister("pisces", MemoryModel, PiscesMemoryModel)
 public:
  PiscesMemoryModel(SST::Params& params, Node* nd);

  virtual ~PiscesMemoryModel();

  std::string toString() const override {
    return "packet flow memory model";
  }

  void access(uint64_t bytes, double max_bw, Callback* cb) override;

  double maxSingleBw() const override {
    return max_single_bw_;
  }

 private:
  void start(int channel, uint64_t size, double max_bw, Callback* cb);
  void channel_free(int channel);
  void data_arrived(int channel, uint32_t bytes);
  Timestamp access(int channel, uint32_t bytes, double max_bw, Callback* cb);
  Timestamp access(int channel, PiscesPacket* pkt, double max_bw, Callback* cb);
  void access(PiscesPacket* pkt, double max_bw, Callback* cb);

 private:
  struct request {
    uint64_t bytes_total;
    uint64_t bytes_arrived;
    double max_bw;
    ExecutionEvent* cb;
    PiscesPacket* pkt;

    request(uint64_t bytes, double bw, Callback* c) :
      bytes_total(bytes), bytes_arrived(0), max_bw(bw), cb(c), pkt(nullptr)
    {
    }

    request(double bw, Callback* c, PiscesPacket* p) :
      max_bw(bw), cb(c), pkt(p)
    {
    }

  };

  std::vector<int> channels_available_;
  std::vector<request> channel_requests_;
  std::list<request, sprockit::thread_safe_allocator<request>> stalled_requests_;

  int nchannels_;
  double max_bw_;
  double max_single_bw_;
  Timestamp latency_;
  PiscesBandwidthArbitrator* arb_;
  int packet_size_;

};

}
} /* namespace sstmac */


#endif // pisces_MEMORY_MODEL_H
