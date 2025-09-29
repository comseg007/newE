
# -*- coding: utf-8 -*-
import sys
import os
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import spacy

nlp = spacy.load("en_core_web_sm")

skip_verbs = {
    "be", "am", "is", "are", "was", "were", "been", "being",
    "do", "does", "did", "done", "doing",
    "have", "has", "had", "having",
    "can", "could", "will", "would", "shall", "should",
    "may", "might", "must", "ought"
}

def newE_by_spaCy(text):
    """
    1. 只将实义动词（非系动词、助动词、情态动词）的过去式/过去分词替换为原形+_ed
    2. 可数名词的复数变为单数（去掉+s）
    3. 取消第三人称主语时实义动词谓语+s（如 eats -> eat）
    """
    doc = nlp(text)
    tokens = []
    for i, token in enumerate(doc):
        # 1. 动词过去式/过去分词处理
        if token.tag_ in ("VBD", "VBN") and token.lemma_.lower() not in skip_verbs:
            tokens.append(token.lemma_ + "_ed" + token.whitespace_)
        # 2. 可数名词复数变单数
        elif token.tag_ == "NNS" and token.lemma_ != token.text:
            tokens.append(token.lemma_ + token.whitespace_)
        # 3. 取消第三人称主语时动词+s
        elif (
            token.tag_ == "VBZ"  # 动词三单
            and token.lemma_.lower() not in skip_verbs
        ):
            tokens.append(token.lemma_ + token.whitespace_)
        else:
            tokens.append(token.text + token.whitespace_)
    return "".join(tokens)

def test_newE():
    test_cases = [
        "I taught him Chinese last year.",
        "He walked to the store and bought some apples.",
        "The cake was eaten by the children.",
        "I could find that.",
        "She has done her homework.",
        "They were being watched.",
        "He is running fast.",
        "We must finish the work.",
        "He eats two apples every morning."
                ]
    
    for i, sentence in enumerate(test_cases, 1):
        result = newE_by_spaCy(sentence)
        print(f"Test {i}:")
        print(f"Original: {sentence}")
        print(f"Processed: {result}\n")

if __name__ == "__main__":
    test_newE()
    