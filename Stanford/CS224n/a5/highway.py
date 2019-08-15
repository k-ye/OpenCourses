#!/usr/bin/env python3
# -*- coding: utf-8 -*-

"""
CS224N 2018-19: Homework 5
"""

# YOUR CODE HERE for part 1h
import torch
import torch.nn as nn
import torch.nn.utils
import torch.nn.functional as F


class Highway(nn.Module):
    def __init__(self, embed_size, dropout_rate):
        """ Init Highway network.

        @param embed_size (int): Embedding size (dimensionality)
        @param dropout_rate (float): Dropout probability, for attention
        """
        super(Highway, self).__init__()

        self.embed_size = embed_size
        self.dropout_rate = dropout_rate

        self.projection = nn.Linear(embed_size, embed_size, bias=True)
        self.gate = nn.Linear(embed_size, embed_size, bias=True)
        self.dropout = nn.Dropout(dropout_rate)

    def forward(self, x_conv_out: torch.Tensor) -> torch.Tensor:
        """ Take a mini-batch of the CNN output, compute the final word embedding.

        @param x_conv_out (torch.Tensor): (batch_size, embed_size)

        @returns x_embed (Tensor): a variable/tensor of shape (batch_size, embed_size) representing the
                                    log-likelihood of generating the gold-standard target sentence for
                                    each example in the input batch. Here b = batch size.
        """
        x_proj = F.relu(self.projection(x_conv_out))
        x_gate = torch.sigmoid(self.gate(x_conv_out))
        x_hway = x_gate * x_proj + (1.0 - x_gate) * x_conv_out
        word_embed = self.dropout(x_hway)
        return word_embed

# END YOUR CODE
