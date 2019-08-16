#!/usr/bin/env python3
# -*- coding: utf-8 -*-

"""
CS224N 2018-19: Homework 5
"""

import torch
import torch.nn as nn
import torch.nn.functional as F


class CharDecoder(nn.Module):
    def __init__(self, hidden_size, char_embedding_size=50, target_vocab=None):
        """ Init Character Decoder.

        @param hidden_size (int): Hidden size of the decoder LSTM
        @param char_embedding_size (int): dimensionality of character embeddings
        @param target_vocab (VocabEntry): vocabulary for the target language. See vocab.py for documentation.
        """
        # YOUR CODE HERE for part 2a
        # TODO - Initialize as an nn.Module.
        # - Initialize the following variables:
        # self.charDecoder: LSTM. Please use nn.LSTM() to construct this.
        # self.char_output_projection: Linear layer, called W_{dec} and b_{dec} in the PDF
        # self.decoderCharEmb: Embedding matrix of character embeddings
        # self.target_vocab: vocabulary for the target language
        ###
        # Hint: - Use target_vocab.char2id to access the character vocabulary for the target language.
        # - Set the padding_idx argument of the embedding matrix.
        # - Create a new Embedding layer. Do not reuse embeddings created in Part 1 of this assignment.
        super(CharDecoder, self).__init__()

        self.charDecoder = nn.LSTM(
            char_embedding_size, hidden_size, bias=True, bidirectional=False)  # should we use dropout?
        char2id = target_vocab.char2id
        self.char_output_projection = nn.Linear(
            hidden_size, len(char2id), bias=True)
        self.decoderCharEmb = nn.Embedding(
            len(char2id), char_embedding_size, padding_idx=char2id['<pad>'])
        self.target_vocab = target_vocab
        # END YOUR CODE

    def forward(self, input, dec_hidden=None):
        """ Forward pass of character decoder.

        @param input: tensor of integers, shape (length, batch)
        @param dec_hidden: internal state of the LSTM before reading the input characters. A tuple of two tensors of shape (1, batch, hidden_size)

        @returns scores: called s_t in the PDF, shape (length, batch, self.vocab_size)
        @returns dec_hidden: internal state of the LSTM after reading the input characters. A tuple of two tensors of shape (1, batch, hidden_size)
        """
        # YOUR CODE HERE for part 2b
        # TODO - Implement the forward pass of the character decoder.
        # (word_length, batch, char_embed_size)
        X = self.decoderCharEmb(input)
        # hiddens:     (word_length, batch, hidden_size)
        # last_hidden: a tuple of two tensors, both size are (1, batch, hidden_size)
        hiddens, last_hidden = self.charDecoder(X, dec_hidden)
        # (word_length, batch, char_vocab_size), do not run softmax on it
        S = self.char_output_projection(hiddens)
        return S, last_hidden

    def train_forward(self, char_sequence, dec_hidden=None):
        """ Forward computation during training.

        @param char_sequence: tensor of integers, shape (length, batch). Note that "length" here and in forward() need not be the same.
        @param dec_hidden: initial internal state of the LSTM, obtained from the output of the word-level decoder. A tuple of two tensors of shape (1, batch, hidden_size)

        @returns The cross-entropy loss, computed as the *sum* of cross-entropy losses of all the words in the batch.
        """
        # YOUR CODE HERE for part 2c
        # TODO - Implement training forward pass.
        ###
        # Hint: - Make sure padding characters do not contribute to the cross-entropy loss.
        # - char_sequence corresponds to the sequence x_1 ... x_{n+1} from the handout (e.g., <START>,m,u,s,i,c,<END>).

        # ((length - 1), batch)
        x_src = char_sequence[:-1, :]
        # ((length - 1), batch)
        x_tgt = char_sequence[1:, :]
        # ((length - 1), batch, char_vocab_size)
        scores, _ = self.forward(x_src, dec_hidden)
        ce_loss = nn.CrossEntropyLoss(reduction='sum')
        # S = F.softmax(S, dim=2)

        # massage the dimensions a bit for CE to work
        # (batch, char_vocab_size, (length - 1))
        scores = scores.permute(1, 2, 0)
        # (batch, (length - 1))
        x_tgt = torch.t(x_tgt)
        loss = ce_loss(scores, x_tgt)
        return loss

    def decode_greedy(self, initialStates, device, max_length=21):
        """ Greedy decoding
        @param initialStates: initial internal state of the LSTM, a tuple of two tensors of size (1, batch, hidden_size)
        @param device: torch.device (indicates whether the model is on CPU or GPU)
        @param max_length: maximum length of words to decode

        @returns decodedWords: a list (of length batch) of strings, each of which has length <= max_length.
                              The decoded strings should NOT contain the start-of-word and end-of-word characters.
        """

        # YOUR CODE HERE for part 2d
        # TODO - Implement greedy decoding.
        # Hints:
        # - Use target_vocab.char2id and target_vocab.id2char to convert between integers and characters
        # - Use torch.tensor(..., device=device) to turn a list of character indices into a tensor.
        # - We use curly brackets as start-of-word and end-of-word characters. That is, use the character '{' for <START> and '}' for <END>.
        # Their indices are self.target_vocab.start_of_word and self.target_vocab.end_of_word, respectively.
        batch_size = initialStates[0].shape[1]
        # (1, batch_size)
        prevCharIdx = torch.tensor(
            [[self.target_vocab.start_of_word] * batch_size], dtype=torch.long, device=device)
        dec_hidden = initialStates
        # (batch_size, max_length)
        output = [[-1] * max_length for _ in range(batch_size)]
        for i in range(max_length):
            # (1, batch_size, char_embed_size)
            charEmbed = self.decoderCharEmb(prevCharIdx)
            # dec_hidden: a 2-tuple of Tensors of (1, batch_size, hidden_size)
            _, dec_hidden = self.charDecoder(charEmbed, dec_hidden)
            # (batch_size, hidden_size)
            h_t = torch.squeeze(dec_hidden[0], dim=0)
            # (batch_size, char_vocab_size)
            S_t = self.char_output_projection(h_t)
            # (batch_size, char_vocab_size)
            p_t = F.softmax(S_t, dim=1)
            # (batch_size)
            charIdx = torch.argmax(p_t, dim=1)
            # (1, batch_size)
            prevCharIdx = torch.unsqueeze(charIdx, dim=0)
            cil = charIdx.tolist()
            for b in range(batch_size):
                output[b][i] = cil[b]
        output = [''.join([self.target_vocab.id2char[i]
                           for i in chIdxs]) for chIdxs in output]

        def truncateWord(w):
            tv = self.target_vocab
            eow = tv.id2char[tv.end_of_word]
            return w.split(eow)[0]
        output = list(map(truncateWord, output))
        return output
        # END YOUR CODE
