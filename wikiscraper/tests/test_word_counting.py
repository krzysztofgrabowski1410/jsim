import unittest
from unittest.mock import MagicMock, patch, mock_open
import json
from src.wiki_scraper import WikiScraper, WORD_COUNTS_FILE


class TestWordCounting(unittest.TestCase):
    def setUp(self):
        self.scraper = WikiScraper()
        # Mock processor to avoid network calls
        self.scraper.processor = MagicMock()

    @patch('builtins.open', new_callable=mock_open, read_data='{}')
    @patch('json.dump')
    @patch('os.path.exists', return_value=True)
    def test_update_word_counts_aggregation(self, mock_exists, mock_json_dump, mock_file):
        # Setup existing data in the mock file
        initial_data = json.dumps({"pikachu": 5, "ash": 2})
        mock_file.side_effect = [
            mock_open(read_data=initial_data).return_value,  # First read
            mock_open().return_value  # Write
        ]

        # New counts to add
        new_counts = {"pikachu": 3, "misty": 4}

        self.scraper._update_word_counts_file(new_counts)

        # Expected data to be written back
        expected_data = {
            "pikachu": 8,  # 5 + 3
            "ash": 2,  # 2 + 0
            "misty": 4  # 0 + 4
        }

        # Verify json.dump was called with aggregated data
        # args[0] is data, args[1] is file handle
        args, _ = mock_json_dump.call_args
        self.assertEqual(args[0], expected_data)

    def test_word_extraction_scope(self):
        from src.article_processor import ArticleProcessor
        processor = ArticleProcessor()

        html = """
        <html>
            <div id="sidebar">Sidebar text should be ignored</div>
            <div class="mw-parser-output">
                <p>Content text.</p>
                <div class="mw-editsection">Edit</div> <!-- Usually text inside is fine, though ideally ignored, requirements say "static page elements" -->
                <p>More content.</p>
            </div>
            <footer>Footer text ignored</footer>
        </html>
        """

        counts = processor.count_words(html)

        # Should include "content", "text", "more"
        self.assertIn("content", counts)
        self.assertIn("more", counts)

        # Should NOT include "sidebar", "footer"
        self.assertNotIn("sidebar", counts)
        self.assertNotIn("footer", counts)


if __name__ == '__main__':
    unittest.main()
