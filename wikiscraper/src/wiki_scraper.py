import argparse
import json
import time
import sys
import os
import matplotlib.pyplot as plt
import pandas as pd
from typing import Dict, List, Optional
from src.article_processor import ArticleProcessor, ArticleNotFound

#Constants
WORD_COUNTS_FILE = "word-counts.json"

# Common English words for frequency analysis (Simplified)
COMMON_ENGLISH_WORDS = {
    "the": 100, "be": 50, "to": 45, "of": 40, "and": 35, "a": 30, "in": 25, "that": 20, "have": 15, "i": 10
}

class WikiScraper:
    """
    Main controller class for the WikiScraper application.
    Encapsulates application logic and state, decoupled from CLI parsing.
    """

    def __init__(self, base_url: Optional[str] = None, offline_path: Optional[str] = None):
        # Initialize main processing class here with custom base_url and an offline_path
        self.processor = ArticleProcessor(base_url=base_url if base_url else "https://bulbapedia.bulbagarden.net/wiki/", offline_path=offline_path)

    def execute(self, args: argparse.Namespace):
        """
        Executes the command based on parsed arguments.
        """
        try:
            if args.command == 'summary':
                self.handle_summary(args.search_term)
            elif args.command == 'table':
                self.handle_table(args.search_term, args.number, args.first_row_is_header)
            elif args.command == 'count-words':
                self.handle_count_words(args.search_term)
            elif args.command == 'analyze-relative-word-frequency':
                self.handle_analysis(args.mode, args.count, args.chart)
            elif args.command == 'auto-count-words':
                self.handle_auto_count(args.initial_search_term, args.depth, args.wait)
        except ArticleNotFound as e:
            print(f"Error: {e}")
            sys.exit(1)
        except Exception as e:
            print(f"Error: {e}", file=sys.stderr)
            sys.exit(1)

    def handle_summary(self, search_term: str):
        html = self.processor.fetch_article(search_term)
        summary = self.processor.get_summary(html)
        print(f"Summary for '{search_term}':\n{summary}")

    def handle_table(self, search_term: str, table_number: int, first_row_is_header: bool):
        html = self.processor.fetch_article(search_term)
        df = self.processor.extract_table(html, table_number, first_row_is_header)

        filename = f"{search_term.replace(' ', '_')}.csv"
        # Save to CSV including the index (row headers)
        df.to_csv(filename, index=True)
        print(f"Table saved to {filename}")

        # Analyze value frequencies (excluding headers/index)
        print("\nValue Frequencies:")
        # Flatten the DataFrame values to a single Series to count frequencies
        if not df.empty:
            frequencies = pd.Series(df.values.ravel()).value_counts()
            # Convert to DataFrame for pretty printing with explicit columns
            freq_df = frequencies.reset_index()
            freq_df.columns = ['Value', 'Count']
            print(freq_df.to_string(index=False))
        else:
            print("Table is empty.")

    def handle_count_words(self, search_term: str, update_file: bool = True) -> Dict[str, int]:
        html = self.processor.fetch_article(search_term)
        counts = self.processor.count_words(html)

        if update_file:
            self._update_word_counts_file(counts)
            print(f"Word counts for '{search_term}' added to {WORD_COUNTS_FILE}")

        return counts

    def _update_word_counts_file(self, new_counts: Dict[str, int]):
        """
        Updates the global word counts file by aggregating new counts.
        """
        data = {}
        if os.path.exists(WORD_COUNTS_FILE):
            try:
                with open(WORD_COUNTS_FILE, 'r') as f:
                    data = json.load(f)
            except json.JSONDecodeError:
                pass  # Start fresh if corrupt

        # Aggregate counts
        for word, count in new_counts.items():
            data[word] = data.get(word, 0) + count

        with open(WORD_COUNTS_FILE, 'w') as f:
            json.dump(data, f, indent=2)

    def handle_analysis(self, mode: str, count: int, chart_path: Optional[str], lang: str = 'en'):
        """
        Analyzes word frequency relative to the language's baseline.
        """
        try:
            global wordfreq
            import wordfreq
        except ImportError:
            print("Error: 'wordfreq' package is required for analysis. Please install it.")
            sys.exit(1)

        if not os.path.exists(WORD_COUNTS_FILE):
            print("No word counts data found. Run count-words first.")
            return

        with open(WORD_COUNTS_FILE, 'r') as f:
            article_counts = json.load(f)

        if not article_counts:
            print("Word counts file is empty.")
            return

        # Convert to DataFrame
        df = pd.DataFrame(list(article_counts.items()), columns=['word', 'count'])

        # Normalize article counts: count / max_count
        max_article_count = df['count'].max()
        df['freq_article'] = df['count'] / max_article_count

        # We normalize language freq by the frequency of 'the' (or top word) in that language
        # wordfreq.word_frequency returns a float (0.0 to 1.0)
        # To find max freq, we can just ask for 'the' or similar common word, or use top_n_list(1)[0]
        try:
            top_word = wordfreq.top_n_list(lang, 1)[0]
            max_lang_freq = wordfreq.word_frequency(top_word, lang)
        except IndexError:
            print(f"Error: Language '{lang}' not supported or has no data.")
            return

        def get_lang_freq(w):
            f = wordfreq.word_frequency(w, lang)
            return f / max_lang_freq if max_lang_freq > 0 else 0

        result_df = pd.DataFrame()

        if mode == 'article':
            # Sort by article frequency (descending)
            df_sorted = df.sort_values('freq_article', ascending=False).head(count)

            # Get language frequencies
            # If wordfreq returns 0.0, we treat it as a gap (NaN) if it's truly unknown,
            # but wordfreq returns 0 for "not in list".
            # Requirement: "contain gaps ... for words that do not occur in chosen dictionary"

            lang_freqs = []
            for w in df_sorted['word']:
                lf = get_lang_freq(w)
                lang_freqs.append(lf if lf > 0 else None)

            df_sorted['freq_lang'] = lang_freqs
            result_df = df_sorted[['word', 'freq_article', 'freq_lang']].copy()

        elif mode == 'language':
            # Sort by language frequency
            top_words = wordfreq.top_n_list(lang, count)

            data = []
            for w in top_words:
                lf = get_lang_freq(w)

                # Check article data
                # If word exists in article_counts, calculate freq, else None
                if w in article_counts:
                    af = article_counts[w] / max_article_count
                else:
                    af = None

                data.append({
                    'word': w,
                    'freq_article': af,
                    'freq_lang': lf
                })

            result_df = pd.DataFrame(data)

        else:
            print(f"Unknown mode: {mode}. Use 'article' or 'language'.")
            return

        # Rename columns for display
        display_df = result_df.rename(columns={
            'freq_article': 'frequency in the article',
            'freq_lang': 'frequency in wiki language'
        })

        print(display_df.to_string(index=False))

        if chart_path:
            self._generate_chart(result_df, chart_path)

    def _generate_chart(self, df: pd.DataFrame, path: str):
        # Prepare data
        words = df['word'].tolist()
        freq_art = df['freq_article'].fillna(0).tolist()  # Fill NaN with 0 for plotting
        freq_lang = df['freq_lang'].fillna(0).tolist()

        x = range(len(words))
        width = 0.35

        fig, ax = plt.subplots(figsize=(12, 6))

        # Plot bars
        ax.bar([i - width / 2 for i in x], freq_art, width, label='Article', color='skyblue')
        ax.bar([i + width / 2 for i in x], freq_lang, width, label='Language', color='salmon')

        # Formatting
        ax.set_ylabel('Normalized Frequency')
        ax.set_title('Relative Word Frequency')
        ax.set_xticks(x)
        ax.set_xticklabels(words, rotation=45, ha='right')
        ax.legend()

        plt.tight_layout()
        plt.savefig(path)
        print(f"Chart saved to {path}")
    def handle_auto_count(self, initial_term: str, depth: int, wait: float):

        """
        Recursively counts words starting from initial_term up to depth n.
        Uses BFS to traverse links.
        """
        def normalize(term):
            return term.replace(' ', '_').split('#')[0]  # Remove anchors

        initial_norm = normalize(initial_term)
        visited = {initial_norm}
        queue = [(initial_norm, 0)]  # (term, depth)

        processed_count = 0

        while queue:
            current_term, current_depth = queue.pop(0)

            print(f"Processing: {current_term} (Depth: {current_depth})")

            try:
                html = self.processor.fetch_article(current_term)

                new_links = []
                if current_depth < depth:
                    links = self.processor.get_links(html)
                    for link in links:
                        link_norm = normalize(link)
                        if link_norm not in visited:
                            visited.add(link_norm)
                            new_links.append(link_norm)
                            queue.append((link_norm, current_depth + 1))

                counts = self.processor.count_words(html)
                self._update_word_counts_file(counts)

                processed_count += 1

                if queue:  # Only wait if there are more items to process
                    time.sleep(wait)

            except ArticleNotFound:
                print(f"Skipping {current_term}: Not found")
            except Exception as e:
                print(f"Failed to process {current_term}: {e}")

        print(f"\nAuto-count finished. Processed {processed_count} articles.")
    def lang_confidence_score(self, word_counts: Dict[str, int], language_words_freq: Dict[str, float]) -> float:
        """
        Evaluates how well text matches a given language.
        Implementation of the specific requested function signature.
        """
        score = 0.0
        total_words = sum(word_counts.values())
        if total_words == 0:
            return 0.0

        for word, count in word_counts.items():
            if word in language_words_freq:
                # Add to score weighted by frequency alignment
                # This is a dummy implementation logic
                score += (count / total_words) * language_words_freq[word]

        return score


def get_arg_parser() -> argparse.ArgumentParser:
    """
    Creates an argument parser for the command line interface.
    """
    parser = argparse.ArgumentParser(description="WikiScraper Tool")
    parser.add_argument('--wiki', type=str,
                        help='Base URL for the wiki (e.g., https://bulbapedia.bulbagarden.net/wiki/)', default=None)

    subparsers = parser.add_subparsers(dest='command', help='Available commands')

    # Summary
    summary_parser = subparsers.add_parser('summary', help='Get first paragraph')
    summary_parser.add_argument('search_term', type=str, help='Wiki search term')

    # Table
    table_parser = subparsers.add_parser('table', help='Extract a table')
    table_parser.add_argument('search_term', type=str, help='Wiki search term')
    table_parser.add_argument('--number', type=int, required=True, help='Table number (1-based)')
    table_parser.add_argument('--first-row-is-header', action='store_true', help='Use first row as header')

    # Count words
    count_parser = subparsers.add_parser('count-words', help='Count words in article')
    count_parser.add_argument('search_term', type=str, help='Wiki search term')

    # Analyze
    analyze_parser = subparsers.add_parser('analyze-relative-word-frequency', help='Analyze word frequency')
    analyze_parser.add_argument('--mode', type=str, required=True, help='Analysis mode')
    analyze_parser.add_argument('--count', type=int, required=True, help='Number of top words to analyze')
    analyze_parser.add_argument('--chart', type=str, help='Path to save chart png')

    # Auto count
    auto_parser = subparsers.add_parser('auto-count-words', help='Recursive word counting')
    auto_parser.add_argument('initial_search_term', type=str, help='Starting page')
    auto_parser.add_argument('--depth', type=int, required=True, help='Recursion depth')
    auto_parser.add_argument('--wait', type=float, required=True, help='Wait time between requests (seconds)')

    return parser


if __name__ == "__main__":
    parser = get_arg_parser()
    args = parser.parse_args()

    if len(sys.argv) == 1:
        parser.print_help()
    else:
        scraper = WikiScraper(base_url=args.wiki)
        scraper.execute(args)
