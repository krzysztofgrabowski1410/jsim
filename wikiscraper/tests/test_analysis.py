import unittest
from unittest.mock import MagicMock, patch, mock_open
import sys
from src.wiki_scraper import WikiScraper


class TestAnalysis(unittest.TestCase):
    def setUp(self):
        self.scraper = WikiScraper()
        # Ensure wordfreq is mocked in sys.modules so import works
        self.mock_wordfreq_module = MagicMock()
        sys.modules['wordfreq'] = self.mock_wordfreq_module

    def tearDown(self):
        del sys.modules['wordfreq']

    @patch('src.wiki_scraper.plt')
    @patch('builtins.open', new_callable=mock_open, read_data='{"pikachu": 10, "the": 5}')
    @patch('os.path.exists', return_value=True)
    def test_analysis_article_mode(self, mock_exists, mock_file, mock_plt):
        # Setup mocks via the sys.modules reference
        self.mock_wordfreq_module.word_frequency.side_effect = lambda w, l: 0.05 if w == 'the' else 0.0
        self.mock_wordfreq_module.top_n_list.return_value = ['the']

        # Configure subplots mock to return (fig, ax) tuple
        mock_fig = MagicMock()
        mock_ax = MagicMock()
        mock_plt.subplots.return_value = (mock_fig, mock_ax)

        self.scraper.handle_analysis(mode='article', count=2, chart_path="test_chart.png")

        # Verify chart generation called
        mock_plt.subplots.assert_called_once()
        mock_plt.savefig.assert_called_with("test_chart.png")

    @patch('src.wiki_scraper.plt')
    @patch('builtins.open', new_callable=mock_open, read_data='{"pikachu": 10}')
    @patch('os.path.exists', return_value=True)
    def test_analysis_language_mode(self, mock_exists, mock_file, mock_plt):
        # Language mode: sorted by language freq
        # Top words: ['the', 'be']
        self.mock_wordfreq_module.top_n_list.return_value = ['the', 'be']
        self.mock_wordfreq_module.word_frequency.side_effect = lambda w, l: 0.05 if w == 'the' else (
            0.02 if w == 'be' else 0)

        self.scraper.handle_analysis(mode='language', count=2, chart_path=None)


if __name__ == '__main__':
    unittest.main()
