1. 注意padding要做的不只是padding；在word length过大时（a5中`max_word_length = 21`)还需将其truncate，否则embedding会炸。
2. 独立生成一个tensor时（如`torch.tensor(...)`，即不利用上游的tensor计算得出），记得设置`device`，否则切到GPU时会炸。