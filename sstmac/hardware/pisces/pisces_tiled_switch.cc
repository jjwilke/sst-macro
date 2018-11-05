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

#include <string>

#include <sstmac/hardware/pisces/pisces_tiled_switch.h>
#include <sstmac/hardware/pisces/pisces_stats.h>
#include <sstmac/hardware/pisces/pisces_nic.h>
#include <sstmac/hardware/topology/structured_topology.h>
#include <sstmac/hardware/nic/nic.h>
#include <sstmac/common/event_manager.h>
#include <sstmac/common/stats/stat_spyplot.h>
#include <sprockit/util.h>
#include <sprockit/sim_parameters.h>
#include <sprockit/keyword_registration.h>

RegisterKeywords(
{ "switch_geometry", "DEPRECATED: array specifying geometry of the switch" },
{ "geometry", "array specifying geometry of the switch" },
{ "row_buffer_size", "the size of the input buffer in each row" },
{ "nrows", "the number of row tiles in a switch" },
{ "ncols", "the number of col tiles in a switch" },
);

namespace sstmac {
namespace hw {

pisces_tiled_switch::pisces_tiled_switch(sprockit::sim_parameters* params,
                                         uint32_t id, event_manager* mgr)
  : pisces_abstract_switch(params, id, mgr)
{
  nrows_ = params->get_int_param("nrows");
  ncols_ = params->get_int_param("ncols");

#if !SSTMAC_INTEGRATED_SST_CORE
  payload_handler_ = new_handler(this, &pisces_tiled_switch::handle_payload);
  ack_handler_ = new_handler(this, &pisces_tiled_switch::handle_credit);
#endif

  init_components(params);
}

pisces_tiled_switch::~pisces_tiled_switch()
{
  for (pisces_demuxer* dm : row_input_demuxers_){
    if (dm) delete dm;
  }
  for (pisces_crossbar* xbar : xbar_tiles_){
    if (xbar) delete xbar;
  }
  for (pisces_muxer* mux : col_output_muxers_){
    if (mux) delete mux;
  }
}

int
pisces_tiled_switch::row_col_to_tile(int row, int col){
  return row*ncols_ + col;
}

void
pisces_tiled_switch::tile_to_row_col(int tile, int& row, int& col){
  row = tile / ncols_;
  col = tile % ncols_;
}

void
pisces_tiled_switch::init_components(sprockit::sim_parameters* params)
{
  if (!xbar_tiles_.empty())
    return;

  sprockit::sim_parameters* demuxer_params = params->get_namespace("input");

  sprockit::sim_parameters* xbar_params = params->get_namespace("xbar");

  sprockit::sim_parameters* muxer_params = params->get_namespace("link");

  int ntiles = nrows_ * ncols_;
  dst_inports_.resize(ntiles);
  row_input_demuxers_.resize(ntiles);
  xbar_tiles_.resize(ntiles);
  col_output_muxers_.resize(ntiles);

  for (int r=0; r < nrows_; ++r){
    for (int c=0; c < ncols_; ++c){
      int tile = row_col_to_tile(r, c);
      pisces_demuxer* dm = new pisces_demuxer(demuxer_params, this, ncols_,
                                              router_->num_vc(), false/*no vc update*/);
      row_input_demuxers_[tile] = dm;

      // divide by num columns to get row for output muxer
      pisces_crossbar* xbar = new pisces_crossbar(xbar_params, this, ncols_, ncols_,
                                                  router_->num_vc(), true/*yes vc update*/);
      xbar->set_stat_collector(xbar_stats_);

      // packet leaves the switch from the muxer
      // credits will arrive back at muxer with global port ids,
      // need to map these to local ports as well as payloads
      pisces_muxer* muxer = new pisces_muxer(muxer_params, this, ncols_,
                                             router_->num_vc(), false/*no vc update*/);

      col_output_muxers_[tile] = muxer;
      xbar_tiles_[tile] = xbar;
    }
  }

  for (int row_dm=0; row_dm < nrows_; ++row_dm){
    pisces_demuxer* demuxer = row_input_demuxers_[row_dm];
    for (int col_dm=0; col_dm < ncols_; ++col_dm){
      for (int col_out=0; col_out < ncols_; ++col_out){
        /**
        int tile_xbar = row_col_to_tile(row_dm, col_dm);
        pisces_crossbar* xbar = xbar_tiles_[tile_xbar];
        auto out_link = allocate_local_link(demuxer->send_latency(), this, xbar->payload_handler());
        demuxer->set_output(demuxer_params,col_dm,0,out_link);

        auto in_link = allocate_local_link(xbar->credit_latency(), this, demuxer->credit_handler());
        xbar->set_input(xbar_params,0,col_dm,in_link);
        */
      }
    }
  }

  //now loop all the xbars and link them up to ouptut muxers
  for (int rx=0; rx < nrows_; ++rx){
    //wire up the crossbars to inputs and outputs
    for (int cx=0; cx < ncols_; ++cx){
      int tile_xbar = row_col_to_tile(rx, cx);
      pisces_crossbar* xbar = xbar_tiles_[tile_xbar];
      //connect current xbar to all the output muxers in the same col
      for (int rm=0; rm < nrows_; ++rm){
        int tile_muxer = row_col_to_tile(rm, cx);
        pisces_muxer* muxer = col_output_muxers_[tile_muxer];
        //use zero-based input ports corresponding to row number for the muxer
        pisces_debug(
         "Connecting %s:%p local port %d to %s:%p local port %d",
          xbar->to_string().c_str(), xbar, rm,
          muxer->to_string().c_str(), muxer, rx);
        auto out_link = allocate_local_link(xbar->send_latency(), this, muxer->payload_handler());
        xbar->set_output(xbar_params, rm, rx, out_link);
        auto in_link = allocate_local_link(muxer->credit_latency(), this, xbar->credit_handler());
        muxer->set_input(muxer_params, rx, rm, in_link);
      }
    }
  }
}

void
pisces_tiled_switch::connect_output(
  sprockit::sim_parameters* params,
  int src_outport,
  int dst_inport,
  event_link* link)
{
  params->add_param_override("num_vc", router_->num_vc());
  pisces_sender* muxer = col_output_muxers_[src_outport];
  muxer->set_output(params, 0, dst_inport, link);
  dst_inports_[src_outport] = dst_inport;
}

void
pisces_tiled_switch::connect_input(
  sprockit::sim_parameters* params,
  int src_outport,
  int dst_inport,
  event_link* link)
{
  int row = dst_inport % nrows_;
  pisces_sender* demuxer = row_input_demuxers_[row];
  demuxer->set_input(params, row, src_outport, link);
}

timestamp
pisces_tiled_switch::send_latency(sprockit::sim_parameters *params) const
{
  return params->get_namespace("link")->get_time_param("send_latency");
}

timestamp
pisces_tiled_switch::credit_latency(sprockit::sim_parameters *params) const
{
  return params->get_namespace("input")->get_time_param("credit_latency");
}

int
pisces_tiled_switch::queue_length(int port) const
{
  spkt_throw_printf(sprockit::unimplemented_error,
    "pisces_tiled_switch::queue_length");
}

void
pisces_tiled_switch::handle_credit(event *ev)
{
  pisces_credit* credit = static_cast<pisces_credit*>(ev);
  pisces_muxer* recver = col_output_muxers_[credit->port()];
  recver->handle_credit(credit);
}

void
pisces_tiled_switch::handle_payload(event *ev)
{
  pisces_packet* payload = static_cast<pisces_packet*>(ev);

  debug_printf(sprockit::dbg::pisces,
               "tiled switch %d: incoming payload %s",
               int(my_addr_), payload->to_string().c_str());

  int row;// = get_row(hdr->arrival_port);
  pisces_demuxer* demuxer = row_input_demuxers_[row];
  //now figure out the new port I am routing to
  router_->route(payload);

  int edge_port = payload->edge_outport();
  int dst_inport = dst_inports_[edge_port];

  payload->reset_stages(get_col(edge_port), get_row(edge_port));

  debug_printf(sprockit::dbg::pisces,
               "tiled switch %d: routed payload %s to port %d, vc %d = %d,%d",
               int(my_addr_), payload->to_string().c_str(),
               payload->edge_outport(), payload->next_vc(),
               get_row(edge_port), get_col(edge_port));
  demuxer->handle_payload(payload);
}

std::string
pisces_tiled_switch::to_string() const
{
  return sprockit::printf("pisces tiled switch %d", int(my_addr_));
}

link_handler*
pisces_tiled_switch::credit_handler(int port)
{
#if SSTMAC_INTEGRATED_SST_CORE
  return new_link_handler(this, &pisces_tiled_switch::handle_credit);
#else
  return ack_handler_;
#endif
}

link_handler*
pisces_tiled_switch::payload_handler(int port)
{
#if SSTMAC_INTEGRATED_SST_CORE
  return new_link_handler(this, &pisces_tiled_switch::handle_payload);
#else
  return payload_handler_;
#endif
}

void
pisces_tiled_switch::deadlock_check()
{
  for (int r=0; r < nrows_; ++r){
    row_input_demuxers_[r]->deadlock_check();
    for (int c=0; c < ncols_; ++c){
      int tile = row_col_to_tile(r, c);
      col_output_muxers_[tile]->deadlock_check();
      xbar_tiles_[tile]->deadlock_check();
    }
  }
}

void
pisces_tiled_switch::deadlock_check(event *ev)
{
  for (int r=0; r < nrows_; ++r){
    row_input_demuxers_[r]->deadlock_check(ev);
    for (int c=0; c < ncols_; ++c){
      int tile = row_col_to_tile(r, c);
      col_output_muxers_[tile]->deadlock_check(ev);
      xbar_tiles_[tile]->deadlock_check(ev);
    }
  }
}

}

}
