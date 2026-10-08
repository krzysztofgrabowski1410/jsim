import json
import wordfreq
import matplotlib.pyplot as plt
import pandas as pd
import numpy as np
import os
from src.article_processor import ArticleProcessor

# Setup Processor
processor = ArticleProcessor()

print("Fetching Long Article...")
try:
    html_long = processor.fetch_article("Pokémon_world")
    long_article_counts = processor.count_words(html_long)
except Exception as e:
    print(f"Error fetching long: {e}")
    long_article_counts = {}

print("Fetching Short/Weird Article...")
try:
    # "List of Pokémon by National Pokédex number" contains mostly names, not much structure words
    html_short = processor.fetch_article("List_of_Pokémon_by_National_Pokédex_number")
    short_article_counts = processor.count_words(html_short)
except Exception as e:
    print(f"Error fetching short: {e}")
    short_article_counts = {}

# External Texts (Simulated)
text_en = """
Python is a high-level, general-purpose programming language. Its design philosophy emphasizes code readability with the use of significant indentation. Python is dynamically typed and garbage-collected. It supports multiple programming paradigms, including structured (particularly procedural), object-oriented and functional programming. It is often described as a "batteries included" language due to its comprehensive standard library.
Guido van Rossum began working on Python in the late 1980s as a successor to the ABC programming language and first released it in 1991 as Python 0.9.0. Python 2.0 was released in 2000. Python 3.0, released in 2008, was a major revision not completely backward-compatible with earlier versions. Python 2.7.18, released in 2020, was the last release of Python 2.
Python consistently ranks as one of the most popular programming languages.
"""

text_de = """
Deutschland ist ein Bundesstaat in Mitteleuropa. Er besteht aus 16 Ländern und ist als freiheitlich-demokratischer und sozialer Rechtsstaat verfasst. Die Bundesrepublik Deutschland stellt die jüngste Ausprägung des deutschen Nationalstaates dar. Deutschland hat rund 84 Millionen Einwohner und zählt bei einer Größe von 357.588 Quadratkilometern mit durchschnittlich 236 Einwohnern pro Quadratkilometer zu den dicht besiedelten Flächenstaaten.
An Deutschland grenzen neun Staaten, es hat Anteil an der Nordsee und an der Ostsee im Norden sowie dem Bodensee und den Alpen im Süden. Es liegt in der gemäßigten Klimazone und verfügt über sechzehn Nationalparks und über hundert Naturparks. Bundeshauptstadt sowie bevölkerungsreichste deutsche Stadt ist Berlin. Weitere Metropolen mit mehr als einer Million Einwohnern sind Hamburg, München und Köln.
"""

text_es = """
España, también denominado Reino de España, es un país soberano transcontinental, miembro de la Unión Europea, constituido en Estado social y democrático de derecho y cuya forma de gobierno es la monarquía parlamentaria. Su territorio, con capital en Madrid, está organizado en diecisiete comunidades autónomas, formadas a su vez por cincuenta provincias, y dos ciudades autónomas.
España se sitúa tanto al sur de Europa Occidental como en el norte de África. En Europa, ocupa la mayor parte de la península ibérica, conocida como España peninsular, y el archipiélago de las islas Baleares (en el mar Mediterráneo occidental); en África se hallan las ciudades de Ceuta (en la península tingitana) y Melilla (en el cabo de Tres Forcas), las islas Canarias (en el océano Atlántico nororiental), las islas Chafarinas (mar Mediterráneo), el peñón de Vélez de la Gomera (mar Mediterráneo), las islas Alhucemas (golfo de las islas Alhucemas) y la isla de Alborán (mar de Alborán).
"""


def get_counts(text):
    import re
    from collections import Counter
    words = re.findall(r'\b\w+\b', text.lower())
    return dict(Counter(words))


ext_counts_en = get_counts(text_en)
ext_counts_de = get_counts(text_de)
ext_counts_es = get_counts(text_es)


# Lang Confidence Function
def lang_confidence_score(word_counts, language_words_list):
    total = sum(word_counts.values())
    if total == 0: return 0.0
    matches = sum(count for word, count in word_counts.items() if word in language_words_list)
    return matches / total


# Experiment
ks = [3, 10, 100, 1000]
languages = ['en', 'de', 'es']
texts = {
    'Wiki Long (EN)': long_article_counts,
    'Wiki Short (EN)': short_article_counts,
    'Ext EN': ext_counts_en,
    'Ext DE': ext_counts_de,
    'Ext ES': ext_counts_es
}

results = {}

for lang in languages:
    results[lang] = {}
    max_k = max(ks)
    top_words_full = wordfreq.top_n_list(lang, max_k)
    top_words_sets = {k: set(top_words_full[:k]) for k in ks}

    for text_name, counts in texts.items():
        scores = []
        for k in ks:
            score = lang_confidence_score(counts, top_words_sets[k])
            scores.append(score)
        results[lang][text_name] = scores

print("Results Computed.")
for lang in languages:
    print(f"\nLanguage: {lang}")
    for text_name, scores in results[lang].items():
        print(f"  {text_name}: {scores}")
