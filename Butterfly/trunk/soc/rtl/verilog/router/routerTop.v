`define HIGH    1'b1
`define LOW     1'b0

`define MSG_DST     127:120
`define MSG_X       126:124
`define MSG_Y       122:120
`define MSG_SRC     119:112
`define MSG_ROUT    111:80
`define MSG_AGE     79:72

module routerTop(X, Y, rst_i, clk_i, cs_i, cyc_i, stb_i, ack_o, we_i, adr_i, dat_i, dat_o, rxdX, rxdY, txdX, txdY);
input [3:0] X;      // router address
input [3:0] Y;      // router address
input rst_i;
input clk_i;
input cs_i;
input cyc_i;
input stb_i;
output ack_o;
input we_i;
input [7:0] adr_i;
input [7:0] dat_i;
output reg [7:0] dat_o;
input rxdX;
input rxdY;
output txdX;
output txdY;

reg rxCycX, rxStbX, rxWeX, rxCsX;
reg rxCycY, rxStbY, rxWeY, rxCsY;
reg txCycX, txStbX, txWeX, txCsX;
reg txCycY, txStbY, txWeY, txCsY;
wire dpX,dpY;
wire rxAckX,rxAckY,txAckX,txAckY;
wire [127:0] rxDatoX,rxDatoY;
reg [127:0] txDatiX, txDatiY;
reg [127:0] rxBufX, rxBufY, txBuf, fifoDati;
wire [127:0] fifoDato;
wire empty;

wire cs = cs_i & cyc_i & stb_i;

reg rdf, wf, dpTx;
reg [3:0] state;
parameter IDLE  = 4'd0;
parameter ACKRXX = 4'd1;
parameter CHK_MATCHX = 4'd2;
parameter NACKTXX = 4'd3;
parameter NACKTXY = 4'd4;
parameter CHK_MATCHY = 4'd5;
parameter ACKRXY = 4'd6;
parameter CHK_MATCHT = 4'd7;
parameter TXGBL = 4'd8;
parameter NACKTXXG = 4'd9;

routerRx u1 (
	// WISHBONE SoC bus interface
	.rst_i(rst_i),			// reset
	.clk_i(clk_i),			// clock
	.cyc_i(rxCycX),			// cycle is valid
	.stb_i(rxStbX),			// strobe
	.ack_o(rxAckX),			// data is ready
	.we_i(rxWeX),				// write (this signal is used to qualify reads)
	.dat_o(rxDatoX),		// data out
	//------------------------
	.cs_i(rxCsX),				// chip select
	.baud16x_ce(1'b1),		// baud rate clock enable (run at full rate)
	.clear(),			// clear reciever
	.rxd(rxdX),				// external serial input
	.data_present(dpX),	// data present in fifo
	.frame_err(),		// framing error
	.overrun()			// receiver overrun
);

routerTx u3 (
	// WISHBONE SoC bus interface
	.rst_i(rst_i),		// reset
	.clk_i(clk_i),		// clock
	.cyc_i(txCycX),		// cycle valid
	.stb_i(txStbX),		// strobe
	.ack_o(txAckX),		// transfer done
	.we_i(txWeX),		// write transmitter
	.dat_i(txDatiX),   // data in
	//--------------------
	.cs_i(txCsX),			// chip select
	.baud16x_ce(1'b1),	// baud rate clock enable
	.cts(),			// clear to send
	.txd(txdX),		// external serial output
	.empty(txEmptyX)	    // buffer is empty
);

routerRx u5 (
	// WISHBONE SoC bus interface
	.rst_i(rst_i),			// reset
	.clk_i(clk_i),			// clock
	.cyc_i(rxCycY),			// cycle is valid
	.stb_i(rxStbY),			// strobe
	.ack_o(rxAckY),			// data is ready
	.we_i(rxWeY),				// write (this signal is used to qualify reads)
	.dat_o(rxDatoY),		// data out
	//------------------------
	.cs_i(rxCsY),				// chip select
	.baud16x_ce(1'b1),		// baud rate clock enable (run at full rate)
	.clear(),			// clear reciever
	.rxd(rxdY),				// external serial input
	.data_present(dpY),	// data present in fifo
	.frame_err(),		// framing error
	.overrun()			// receiver overrun
);

routerTx u7 (
	// WISHBONE SoC bus interface
	.rst_i(rst_i),		// reset
	.clk_i(clk_i),		// clock
	.cyc_i(txCycY),		// cycle valid
	.stb_i(txStbY),		// strobe
	.ack_o(txAckY),		// transfer done
	.we_i(txWeY),		// write transmitter
	.dat_i(txDatiY),   // data in
	//--------------------
	.cs_i(txCsY),			// chip select
	.baud16x_ce(1'b1),	// baud rate clock enable
	.cts(),			// clear to send
	.txd(txdY),		// external serial output
	.empty(txEmptyY)	    // buffer is empty
);

routerFifo ufifo
(
  .clk(clk_i),
  .srst(rst_i),
  .din(fifoDati),
  .wr_en(wf),
  .rd_en(rdf),
  .dout(fifoDato),
  .full(),
  .almost_full(),
  .empty(empty),
  .almost_empty()
);

always @(posedge clk_i)
if (rst_i) begin
    wf <= 1'b0;
    dpTx <= 1'b0;
end
else begin
wf <= 1'b0;
rdf <= 1'b0;
    if (cs & we_i && adr_i[4:0]==5'h12)
        dpTx <= 1'b1;
    if (cs & we_i) begin
        case(adr_i[4:0])
        5'd0:   txBuf[7:0] <= dat_i;
        5'd1:   txBuf[15:8] <= dat_i;
        5'd2:   txBuf[23:16] <= dat_i;
        5'd3:   txBuf[31:24] <= dat_i;
        5'd4:   txBuf[39:32] <= dat_i;
        5'd5:   txBuf[47:40] <= dat_i;
        5'd6:   txBuf[55:48] <= dat_i;
        5'd7:   txBuf[63:56] <= dat_i;
        5'd8:   txBuf[71:64] <= dat_i;
        5'd9:   txBuf[79:72] <= dat_i;
        5'hA:   txBuf[87:80] <= dat_i;
        5'hB:   txBuf[95:88] <= dat_i;
        5'hC:   txBuf[103:96] <= dat_i;
        5'hD:   txBuf[111:104] <= dat_i;
        5'hE:   txBuf[119:112] <= dat_i;
        5'hF:   txBuf[127:120] <= dat_i;
        5'h10:  rdf <= 1'b1;
        endcase
    end
    casex(adr_i[4:0])
    5'b0x:  dat_o <= fifoDato >> {adr_i[3:0],3'b0};
    5'h10:  dat_o <= ~empty;
    5'h12:  dat_o <= dpTx;
    endcase
case(state)
IDLE:
    if (dpX) begin
        rxCycX <= `HIGH;
        rxStbX <= `HIGH;
        rxWeX <= `LOW;
        rxCsX <= `HIGH;
        state <= ACKRXX;
    end
    else if (dpY) begin
        rxCycY <= `HIGH;
        rxStbY <= `HIGH;
        rxWeY <= `LOW;
        rxCsY <= `HIGH;
        state <= ACKRXY;
    end
    else if (dpTx) begin
        state <= CHK_MATCHT;
    end
ACKRXX:
if (rxAckX) begin
    rxCycX <= `LOW;
    rxStbX <= `LOW;
    rxWeX <= `LOW;
    rxCsX <= `LOW;
    rxBufX <= rxDatoX;
    if (rxBufX[`MSG_AGE] > 8'h40)
        state <= IDLE;
    else begin
        rxBufX[`MSG_AGE] <= rxBufX[`MSG_AGE] + 8'd1;
        state <= CHK_MATCHX;
    end
end
CHK_MATCHX:
    if (rxBufX[`MSG_DST]==8'hFF) begin
        txBuf <= rxBufX;
        state <= TXGBL;
    end
    else if (rxBufX[`MSG_X]!=X) begin
        if (txEmptyX) begin
            txCycX <= `HIGH;
            txStbX <= `HIGH;
            txWeX <= `HIGH;
            txCsX <= `HIGH;
            txDatiX <= rxBufX;
            txDatiX[`MSG_ROUT] <= {rxBufX[`MSG_ROUT],2'b01};
            state <= NACKTXX;
        end
    end
    else if (rxBufX[`MSG_Y]!=Y) begin
        if (txEmptyY) begin
            txCycY <= `HIGH;
            txStbY <= `HIGH;
            txWeY <= `HIGH;
            txCsY <= `HIGH;
            txDatiY <= rxBufX;
            txDatiY[`MSG_ROUT] <= {rxBufX[`MSG_ROUT],2'b10};
            state <= NACKTXY;
        end
    end
    else begin
        wf <= 1'b1;
        fifoDati <= rxBufX;
        state <= IDLE;
    end
NACKTXX:
    begin
        txCycX <= `LOW;
        txStbX <= `LOW;
        txWeX <= `LOW;
        txCsX <= `LOW;
        state <= IDLE;        
    end
NACKTXY:
    begin
        txCycY <= `LOW;
        txStbY <= `LOW;
        txWeY <= `LOW;
        txCsY <= `LOW;
        state <= IDLE;        
    end
ACKRXY:
    if (rxAckY) begin
        rxCycY <= `LOW;
        rxStbY <= `LOW;
        rxWeY <= `LOW;
        rxCsY <= `LOW;
        rxBufY <= rxDatoY;
        if (rxBufY[`MSG_AGE] > 8'h40)
            state <= IDLE;
        else begin
            rxBufY[`MSG_AGE] <= rxBufY[`MSG_AGE] + 8'd1;
            state <= CHK_MATCHY;
        end
    end
CHK_MATCHY:
    if (rxBufY[`MSG_DST]==8'hFF) begin
        txBuf <= rxBufY;
        state <= TXGBL;
    end
    else if (rxBufY[`MSG_X]!=X) begin
        if (txEmptyX) begin
            txCycX <= `HIGH;
            txStbX <= `HIGH;
            txWeX <= `HIGH;
            txCsX <= `HIGH;
            txDatiX <= rxBufY;
            txDatiX[`MSG_ROUT] <= {rxBufY[`MSG_ROUT],2'b01};
            state <= NACKTXX;
        end
    end
    else if (rxBufY[`MSG_Y]!=Y) begin
        if (txEmptyY) begin
            txCycY <= `HIGH;
            txStbY <= `HIGH;
            txWeY <= `HIGH;
            txCsY <= `HIGH;
            txDatiY <= rxBufY;
            txDatiY[`MSG_ROUT] <= {rxBufY[`MSG_ROUT],2'b10};
            state <= NACKTXY;
        end
    end
    else begin
        wf <= 1'b1;
        fifoDati <= rxBufY;
        state <= IDLE;
    end
CHK_MATCHT:
    if (txBuf[`MSG_X]!=X) begin
        if (txEmptyX) begin
            txCycX <= `HIGH;
            txStbX <= `HIGH;
            txWeX <= `HIGH;
            txCsX <= `HIGH;
            txDatiX <= txBuf;
            txDatiX[`MSG_ROUT] <= {txBuf[`MSG_ROUT],2'b01};
            state <= NACKTXX;
            dpTx <= 1'b0;
        end
    end
    else if (txBuf[`MSG_Y]!=Y) begin
        if (txEmptyY) begin
            txCycY <= `HIGH;
            txStbY <= `HIGH;
            txWeY <= `HIGH;
            txCsY <= `HIGH;
            txDatiY <= txBuf;
            txDatiY[`MSG_ROUT] <= {txBuf[`MSG_ROUT],2'b10};
            state <= NACKTXY;
            dpTx <= 1'b0;
        end
    end
    else begin
        wf <= 1'b1;
        fifoDati <= txBuf;
        state <= IDLE;
        dpTx <= 1'b0;
    end
TXGBL:
    if (txEmptyX) begin
        txCycX <= `HIGH;
        txStbX <= `HIGH;
        txWeX <= `HIGH;
        txCsX <= `HIGH;
        txDatiX <= txBuf;
        txDatiX[`MSG_ROUT] <= {txBuf[`MSG_ROUT],2'b01};
        state <= NACKTXXG;
    end
NACKTXXG:
    begin
        txCycX <= `LOW;
        txStbX <= `LOW;
        txWeX <= `LOW;
        txCsX <= `LOW;
        if (txEmptyY) begin
            txCycY <= `HIGH;
            txStbY <= `HIGH;
            txWeY <= `HIGH;
            txCsY <= `HIGH;
            txDatiY <= txBuf;
            txDatiY[`MSG_ROUT] <= {txBuf[`MSG_ROUT],2'b10};
            state <= NACKTXY;
        end
    end

endcase
end

endmodule