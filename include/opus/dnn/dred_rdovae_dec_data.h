/* Auto generated from checkpoint checkpoint_epoch_1.pth */


#ifndef DRED_RDOVAE_DEC_DATA_H
#define DRED_RDOVAE_DEC_DATA_H

#include "nnet.h"


#include "opus_types.h"

#include "dred_rdovae.h"

#include "dred_rdovae_constants.h"


#define DEC_DENSE1_OUT_SIZE 96

#define DEC_GLU1_OUT_SIZE 64

#define DEC_GLU2_OUT_SIZE 64

#define DEC_GLU3_OUT_SIZE 64

#define DEC_GLU4_OUT_SIZE 64

#define DEC_GLU5_OUT_SIZE 64

#define DEC_HIDDEN_INIT_OUT_SIZE 128

#define DEC_OUTPUT_OUT_SIZE 80

#define DEC_CONV_DENSE1_OUT_SIZE 32

#define DEC_CONV_DENSE2_OUT_SIZE 32

#define DEC_CONV_DENSE3_OUT_SIZE 32

#define DEC_CONV_DENSE4_OUT_SIZE 32

#define DEC_CONV_DENSE5_OUT_SIZE 32

#define DEC_GRU_INIT_OUT_SIZE 320

#define DEC_GRU1_OUT_SIZE 64

#define DEC_GRU1_STATE_SIZE 64

#define DEC_GRU2_OUT_SIZE 64

#define DEC_GRU2_STATE_SIZE 64

#define DEC_GRU3_OUT_SIZE 64

#define DEC_GRU3_STATE_SIZE 64

#define DEC_GRU4_OUT_SIZE 64

#define DEC_GRU4_STATE_SIZE 64

#define DEC_GRU5_OUT_SIZE 64

#define DEC_GRU5_STATE_SIZE 64

#define DEC_CONV1_OUT_SIZE 32

#define DEC_CONV1_IN_SIZE 32

#define DEC_CONV1_STATE_SIZE (32 * (1))

#define DEC_CONV2_OUT_SIZE 32

#define DEC_CONV2_IN_SIZE 32

#define DEC_CONV2_STATE_SIZE (32 * (1))

#define DEC_CONV3_OUT_SIZE 32

#define DEC_CONV3_IN_SIZE 32

#define DEC_CONV3_STATE_SIZE (32 * (1))

#define DEC_CONV4_OUT_SIZE 32

#define DEC_CONV4_IN_SIZE 32

#define DEC_CONV4_STATE_SIZE (32 * (1))

#define DEC_CONV5_OUT_SIZE 32

#define DEC_CONV5_IN_SIZE 32

#define DEC_CONV5_STATE_SIZE (32 * (1))

struct RDOVAEDec {
    LinearLayer dec_dense1;
    LinearLayer dec_glu1;
    LinearLayer dec_glu2;
    LinearLayer dec_glu3;
    LinearLayer dec_glu4;
    LinearLayer dec_glu5;
    LinearLayer dec_hidden_init;
    LinearLayer dec_output;
    LinearLayer dec_conv_dense1;
    LinearLayer dec_conv_dense2;
    LinearLayer dec_conv_dense3;
    LinearLayer dec_conv_dense4;
    LinearLayer dec_conv_dense5;
    LinearLayer dec_gru_init;
    LinearLayer dec_gru1_input;
    LinearLayer dec_gru1_recurrent;
    LinearLayer dec_gru2_input;
    LinearLayer dec_gru2_recurrent;
    LinearLayer dec_gru3_input;
    LinearLayer dec_gru3_recurrent;
    LinearLayer dec_gru4_input;
    LinearLayer dec_gru4_recurrent;
    LinearLayer dec_gru5_input;
    LinearLayer dec_gru5_recurrent;
    LinearLayer dec_conv1;
    LinearLayer dec_conv2;
    LinearLayer dec_conv3;
    LinearLayer dec_conv4;
    LinearLayer dec_conv5;
};

int init_rdovaedec(RDOVAEDec *model, const WeightArray *arrays);

#endif /* DRED_RDOVAE_DEC_DATA_H */
