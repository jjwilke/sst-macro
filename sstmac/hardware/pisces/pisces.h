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

#ifndef PACKETFLOW_H
#define PACKETFLOW_H

#include <sstmac/hardware/common/packet.h>
#include <sstmac/common/messages/sst_message.h>
#include <sprockit/thread_safe_new.h>
#include <sprockit/factories/factory.h>
#include <sprockit/debug.h>

DeclareDebugSlot(pisces)
DeclareDebugSlot(pisces_queue)
DeclareDebugSlot(pisces_config)

namespace sstmac {
namespace hw {

/**
 @class pisces
 Encapsulates a group of machine packets traveling together on the
 same path between endpoints.  This is usually one fraction of
 a larger message.
 */
class pisces_packet :
  public packet,
  public sprockit::thread_safe_new<pisces_packet>
{
 public:
  static const double uninitialized_bw;

  ImplementSerializable(pisces_packet)

 public:
  pisces_packet(
    serializable* msg,
    uint32_t num_bytes,
    uint64_t flow_id,
    bool is_tail,
    node_id fromaddr,
    node_id toaddr);

  std::string to_string() const override;

  virtual ~pisces_packet() {}

  /**
    Needed because of routable_message ambiguity
  */
  int vc() const {
    return current_vc_;
  }

  /**
   * @brief reset_stages Configure the internal ports to traverse on a switch
   * @param port0
   */
  void reset_stages(uint8_t port0){
    stage_ = 0;
    outports_[0] = port0;
  }

  void reset_stages(uint8_t port0, uint8_t port1){
    reset_stages(port0);
    outports_[1] = port1;
  }

  void reset_stages(uint8_t port0, uint8_t port1, uint8_t port2){
    reset_stages(port0, port1);
    outports_[2] = port2;
  }

  void advance_stage(){
    inport_ = outports_[stage_];
    ++stage_;
  }

  uint8_t stage() const {
    return stage_;
  }

  int next_local_outport() const {
    return outports_[stage_];
  }

  int next_local_inport() const {
    return inport_;
  }

  int next_vc() const {
    return rtr_header<packet::header>()->deadlock_vc;
  }

  void update_vc() {
    current_vc_ = next_vc();
  }

  void set_inport(int port) {
    inport_ = port;
  }

  timestamp arrival() const {
    return arrival_;
  }

  void set_arrival(timestamp time) {
    arrival_ = time;
  }

  void init_bw(double bw) {
    bw_ = bw_ == uninitialized_bw ? bw : bw_;
  }

  void set_max_bw(double bw){
    init_bw(bw);
    bw_ = std::min(bw_, bw);
  }

  /**
   @return The bandwidth in number of bytes per second
   */
  double bw() const {
    return bw_;
  }

  /**
   @param The bandwidth in number of bytes per second
   */
  void set_bw(double bw) {
    bw_ = bw;
  }

  /**
   * @brief max_incoming_bw The maximum bandwidth a packet
   *        could have on its current component (always > #bw())
   * @return
   */
  double max_incoming_bw() const {
    return max_in_bw_;
  }

  void set_max_incoming_bw(double bw) {
    max_in_bw_ = bw;
  }

  double ser_delay() const {
    return byte_length() / bw_;
  }

  void serialize_order(serializer& ser) override;

 private:
  pisces_packet(){} //for serialization

  double bw_;

  double max_in_bw_;

  timestamp arrival_;

  int current_vc_;

  uint8_t stage_;

  uint8_t outports_[3];

  uint16_t inport_;

};

class pisces_credit :
  public event,
  public sprockit::printable,
  public sprockit::thread_safe_new<pisces_credit>
{

 public:
  ImplementSerializable(pisces_credit)

 public:
  pisces_credit(){} //for serialization

  pisces_credit(
    int port,
    int vc,
    int num_credits)
    : port_(port),
      num_credits_(num_credits),
      vc_(vc)
  {
  }

  int vc() const {
    return vc_;
  }

  int port() const {
    return port_;
  }

#if !SSTMAC_INTEGRATED_SST_CORE
  bool is_payload() const override {
    return false;
  }

  bool is_ack() const override {
    return true;
  }
#endif

  int num_credits() const {
    return num_credits_;
  }

  std::string to_string() const override;

  void serialize_order(serializer& ser) override;

 protected:
  int num_credits_;
  int port_;
  int vc_;


};

}
}


#endif // PACKETFLOW_H
