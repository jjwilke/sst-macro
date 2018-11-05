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

#ifndef pisces_BUFFER_H
#define pisces_BUFFER_H

#include <sstmac/hardware/pisces/pisces_crossbar.h>
#include <sstmac/hardware/pisces/pisces_sender.h>

namespace sstmac {
namespace hw {


class pisces_buffer :
  public pisces_sender
{

 public:
  virtual ~pisces_buffer();

  void set_output(sprockit::sim_parameters* params,
    int this_outport, int dst_inport,
    event_link* link) override;

  void set_input(
    sprockit::sim_parameters* params,
    int this_inport, int src_outport,
    event_link* link) override;

  void handle_credit(event* ev) override;

  void handle_payload(event* ev) override;

  bool space_to_send(int vc, int bytes){
    return credits_[vc] >= bytes;
  }

  int num_credit(int vc) const {
    return credits_[vc];
  }

  event_handler* payload_handler() const {
    return payload_handler_;
  }

  std::string pisces_name() const override {
    return input_.link ? "buffer" : "injection";
  }

  int queue_length() const;

  void deadlock_check() override;

  void deadlock_check(event* ev) override;

  pisces_buffer(sprockit::sim_parameters* params, event_scheduler* parent, int num_vc);

 private:
  input input_;
  output output_;
  uint32_t bytes_delayed_;

 int num_vc_;
 std::vector<payload_queue> queues_;
 std::vector<int> credits_;
#if SSTMAC_SANITY_CHECK
 std::vector<int> initial_credits_;
#endif

 void build_blocked_messages();

 pisces_bandwidth_arbitrator* arb_;
 std::set<int> deadlocked_channels_;
 std::map<int, std::list<pisces_packet*> > blocked_messages_;
 int packet_size_;
 event_handler* payload_handler_;

};

class pisces_endpoint :
  public pisces_sender
{
 public:
  pisces_endpoint(sprockit::sim_parameters* params,
                  event_scheduler* parent,
                  event_handler* output);

  ~pisces_endpoint();

  void handle_credit(event* ev) override;

  void handle_payload(event* ev) override;

  std::string pisces_name() const override {
    return "endpoint";
  }

  void set_output(sprockit::sim_parameters* params,
    int this_outport, int dst_inport,
    event_link* link) override;

  void set_input(
    sprockit::sim_parameters* params,
    int this_inport, int src_outport,
    event_link* link) override {
    input_.port_to_credit = src_outport;
    input_.link = link;
  }

 private:
  input input_;
  event_handler* output_handler_;

};

}
}


#endif // BUFFER_H
