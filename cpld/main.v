/*
 * phloppy_0 - Commodore Amiga floppy drive emulator
 * Copyright (C) 2016-2018 Piotr Wiszowaty
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

`timescale 1ns / 1ps
module main(
    output fdd_sel0_emu,
    output fdd_sel1_emu,
    output fdd_mtr0_emu,
    output chng_emu,
    output index_emu,
    output trk0_emu,
    output wprot_emu,
    output dkrd_emu,
    output rdy_emu,
    input sel0,
    input sel1,
    input sel2,
    input sel3,
    input mtr0,
    input dir,
    input step,
    input dkwdb,
    input dkweb,
    input side,
    input ena0,
    input ena1,
    input ena2,
    input ena3,
    input dkrd_uc,
    input wprot_uc,
    input index_uc,
    output sel0_uc,
    output sel1_uc,
    output dir_uc,
    output step_uc,
    output dkwdb_uc,
    output dkweb_uc,
    output side_uc,
    output emu_vcc_sense,
    input vcc_sense,
    input xclk,
    input flop0_trk0,
    input flop1_trk0,
    input flop2_trk0,
    input flop3_trk0,
    input debug2_uc,
    input debug3_uc,
    output debug2,
    output debug3,
    output debug4);

wire sel2_uc;
wire sel3_uc;
wire emu_sel = ~(sel0_uc & sel1_uc & sel2_uc & sel3_uc);

reg ena0_t = 0;
reg ena0_tt = 0;
reg ena1_t = 0;
reg ena1_tt = 0;
reg ena2_t = 0;
reg ena2_tt = 0;
reg ena3_t = 0;
reg ena3_tt = 0;
reg step_t = 1;
reg step_tt = 1;
reg sel0_t = 1;
reg sel0_tt = 1;
reg sel1_t = 1;
reg sel1_tt = 1;
reg sel2_t = 1;
reg sel2_tt = 1;
reg sel3_t = 1;
reg sel3_tt = 1;

reg chng0_emu = 1;
reg chng1_emu = 1;
reg chng2_emu = 1;
reg chng3_emu = 1;
reg chng0_phys = 1;
reg chng1_phys = 1;
reg chng2_phys = 1;
reg chng3_phys = 1;

function rising_edge;
    input t, tt;
    begin
        rising_edge = ~tt & t;
    end
endfunction

function falling_edge;
    input t, tt;
    begin
        falling_edge = tt & ~t;
    end
endfunction

always @(posedge xclk) begin
    {ena0_t, ena0_tt} <= {ena0, ena0_t};
    {ena1_t, ena1_tt} <= {ena1, ena1_t};
    {ena2_t, ena2_tt} <= {ena2, ena2_t};
    {ena3_t, ena3_tt} <= {ena3, ena3_t};
    {step_t, step_tt} <= {step, step_t};
    {sel0_t, sel0_tt} <= {sel0, sel0_t};
    {sel1_t, sel1_tt} <= {sel1, sel1_t};
    {sel2_t, sel2_tt} <= {sel2, sel2_t};
    {sel3_t, sel3_tt} <= {sel3, sel3_t};

    if (rising_edge(ena0_t, ena0_tt))
        chng0_emu <= 0;
    else if (ena0_tt & ~sel0_tt & rising_edge(step_t, step_tt))
        chng0_emu <= 1;

    if (falling_edge(ena0_t, ena0_tt))
        chng0_phys <= 0;
    else if (~ena0_tt & ~sel0_tt & rising_edge(step_t, step_tt))
        chng0_phys <= 1;

    if (rising_edge(ena1_t, ena1_tt))
        chng1_emu <= 0;
    else if (ena1_tt & ~sel1_tt & rising_edge(step_t, step_tt))
        chng1_emu <= 1;

    if (falling_edge(ena1_t, ena1_tt))
        chng1_phys <= 0;
    else if (~ena1_tt & ~sel1_tt & rising_edge(step_t, step_tt))
        chng1_phys <= 1;

    if (rising_edge(ena2_t, ena2_tt))
        chng2_emu <= 0;
    else if (ena2_tt & ~sel2_tt & rising_edge(step_t, step_tt))
        chng2_emu <= 1;

    if (falling_edge(ena2_t, ena2_tt))
        chng2_phys <= 0;
    else if (~ena2_tt & ~sel2_tt & rising_edge(step_t, step_tt))
        chng2_phys <= 1;

    if (rising_edge(ena3_t, ena3_tt))
        chng3_emu <= 0;
    else if (ena3_tt & ~sel3_tt & rising_edge(step_t, step_tt))
        chng3_emu <= 1;

    if (falling_edge(ena3_t, ena3_tt))
        chng3_phys <= 0;
    else if (~ena3_tt & ~sel3_tt & rising_edge(step_t, step_tt))
        chng3_phys <= 1;
end

assign fdd_sel0_emu = ~vcc_sense | (ena0 ? 1 : sel0);
assign fdd_sel1_emu = ~vcc_sense | 1;
assign fdd_mtr0_emu = ~vcc_sense | (ena0 ? 1 : mtr0);
assign chng_emu     = ~vcc_sense | (~sel0_uc ? chng0_emu :
                                    ~sel1_uc ? chng1_emu :
                                    ~sel2_uc ? chng2_emu :
                                    ~sel3_uc ? chng3_emu :
                                    ~sel0 ? chng0_phys :
                                    ~sel1 ? chng1_phys :
                                    ~sel2 ? chng2_phys :
                                    ~sel3 ? chng3_phys :
                                    1);
assign index_emu    = ~vcc_sense | (emu_sel ? index_uc : 1);
assign trk0_emu     = ~vcc_sense | (~sel0_uc ? flop0_trk0 : ~sel1_uc ? flop1_trk0 : ~sel2_uc ? flop2_trk0 : ~sel3_uc ? flop3_trk0 : 1);
assign wprot_emu    = ~vcc_sense | (emu_sel ? wprot_uc : 1);
assign dkrd_emu     = ~vcc_sense | (emu_sel ? dkrd_uc : 1);
assign rdy_emu      = ~vcc_sense | (emu_sel ? 0 : 1);

assign sel0_uc = ena0 ? sel0 : 1;
assign sel1_uc = ena1 ? sel1 : 1;
assign sel2_uc = ena2 ? sel2 : 1;
assign sel3_uc = ena3 ? sel3 : 1;
assign dir_uc = emu_sel ? dir : 1;
assign step_uc = emu_sel ? step : 1;
assign dkwdb_uc = emu_sel ? dkwdb : 1;
assign dkweb_uc = emu_sel ? dkweb : 1;
assign side_uc = emu_sel ? side : 1;
assign emu_vcc_sense = vcc_sense;

assign debug2 = debug2_uc;
assign debug3 = debug3_uc;
assign debug4 = dir;

endmodule
