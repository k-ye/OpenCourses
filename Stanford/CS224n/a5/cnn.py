#!/usr/bin/env python3
# -*- coding: utf-8 -*-

"""
CS224N 2018-19: Homework 5
"""

# YOUR CODE HERE for part 1i

import torch
import torch.nn as nn
import torch.nn.utils
import torch.nn.functional as F

class CNN(nn.Module):
    def __init__(self, embed_size, num_out_channels, kernel_size):
        """
        @embed_size (int): number of input channels
        @num_out_channels (int): number of output channels. Should be equal to embed_size
        @kernel_size (int): kernal size
        """
        super(CNN, self).__init__()

        self.embed_size = embed_size
        self.conv = nn.Conv1d(embed_size, num_out_channels, kernel_size)

    def forward(self, x_reshaped: torch.Tensor) -> torch.Tensor:
        """ Take a mini-batch of the reshaped char embedding, compute the final word embedding.

        @param x_reshaped (torch.Tensor): (batch_size, embed_size, m_word)

        @returns x_embed (Tensor): a variable/tensor of shape (batch_size, embed_size).
        """
        # batch_size x embed_size x (m_word - kernel_size + 1)
        x_conv = self.conv(x_reshaped)
        x_conv_out = F.max_pool1d(F.relu(x_conv), x_conv.shape[2])
        x_conv_out = torch.squeeze(x_conv_out, dim=2)
        return x_conv_out

# END YOUR CODE
