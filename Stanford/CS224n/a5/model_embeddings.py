#!/usr/bin/env python3
# -*- coding: utf-8 -*-

"""
CS224N 2018-19: Homework 5
model_embeddings.py: Embeddings for the NMT model
Pencheng Yin <pcyin@cs.cmu.edu>
Sahil Chopra <schopra8@stanford.edu>
Anand Dhoot <anandd@stanford.edu>
Michael Hahn <mhahn2@stanford.edu>
"""

import torch
import torch.nn as nn

# Do not change these imports; your module names should be
#   `CNN` in the file `cnn.py`
#   `Highway` in the file `highway.py`
# Uncomment the following two imports once you're ready to run part 1(j)

from cnn import CNN
from highway import Highway

# End "do not change"


class ModelEmbeddings(nn.Module):
    """
    Class that converts input words to their CNN-based embeddings.
    """

    def __init__(self, embed_size, vocab):
        """
        Init the Embedding layer for one language
        @param embed_size (int): Embedding size (dimensionality) for the output 
        @param vocab (VocabEntry): VocabEntry object. See vocab.py for documentation.
        """
        super(ModelEmbeddings, self).__init__()

        # A4 code
        # pad_token_idx = vocab.src['<pad>']
        # self.embeddings = nn.Embedding(len(vocab.src), embed_size, padding_idx=pad_token_idx)
        # End A4 code

        # YOUR CODE HERE for part 1j
        self.embed_size = embed_size
        # char embedding
        pad_token_idx = vocab.char2id['<pad>']
        self.embeddings = nn.Embedding(len(vocab.char2id), embed_size, padding_idx=pad_token_idx)
        # CNN
        kernel_size = 5
        self.cnn = CNN(embed_size, embed_size, kernel_size)
        # Highway
        dropout_rate = 0.3
        self.highway = Highway(embed_size, dropout_rate)
        # END YOUR CODE

    def forward(self, input):
        """
        Looks up character-based CNN embeddings for the words in a batch of sentences.
        @param input: Tensor of integers of shape (sentence_length, batch_size, max_word_length) where
            each integer is an index into the character vocabulary

        @param output: Tensor of shape (sentence_length, batch_size, embed_size), containing the 
            CNN-based embeddings for each word of the sentences in the batch
        """
        # A4 code
        # output = self.embeddings(input)
        # return output
        # End A4 code

        # YOUR CODE HERE for part 1j
        # (sentence_length, batch_size, max_word_length, embed_size)
        sents_embed = self.embeddings(input)
        # (sentence_length, batch_size, embed_size, max_word_length)
        sents_reshaped = torch.transpose(sents_embed, 2, 3)
        output = torch.zeros(sents_reshaped.shape[:3], device=input.device)
        # print(f'sents_reshaped.device={sents_reshaped.device} output.device={output.device}')
        for i, x_reshaped in enumerate(torch.split(sents_reshaped, 1, dim=0)):
            # (batch_size, embed_size, max_word_length)
            x_reshaped = torch.squeeze(x_reshaped, dim=0)
            # (batch_size, embed)
            x_conv_out = self.cnn(x_reshaped)
            # (batch_size, embed)
            x_word_embed = self.highway(x_conv_out)
            output[i, :, :] = x_word_embed
        return output
