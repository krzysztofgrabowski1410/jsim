import unittest
from unittest.mock import MagicMock, patch
from src.article_processor import ArticleProcessor


class TestArticleProcessor(unittest.TestCase):
    def setUp(self):
        self.processor = ArticleProcessor()

    @patch('src.article_processor.requests.Session.get')
    def test_fetch_article_success(self, mock_get):
        mock_response = MagicMock()
        mock_response.status_code = 200
        mock_response.text = "<html></html>"
        mock_get.return_value = mock_response

        html = self.processor.fetch_article("Test Term")
        self.assertEqual(html, "<html></html>")

        # Verify URL formatting
        # Only check if the base url and term are present, avoiding strict equality check on the full call args if generic
        args, _ = mock_get.call_args
        self.assertIn("Test_Term", args[0])

    @patch('src.article_processor.requests.Session.get')
    def test_fetch_article_failure(self, mock_get):
        # Configure the mock to raise a RequestException
        import requests
        mock_get.side_effect = requests.exceptions.RequestException("Connection error")

        with self.assertRaises(ConnectionError):
            self.processor.fetch_article("Bad Term")

    def test_get_summary_no_content(self):
        html = "<html><body>Nothing here</body></html>"
        summary = self.processor.get_summary(html)
        self.assertIn("Could not find content area", summary)

    def test_extract_table_invalid_index(self):
        html = "<html><body><table></table></body></html>"
        with self.assertRaises(ValueError):
            self.processor.extract_table(html, 2)

    def test_count_words_logic(self):
        # Must include content div for new logic
        html = "<html><body><div class='mw-parser-output'><p>Hello world hello</p></div></body></html>"
        counts = self.processor.count_words(html)
        self.assertEqual(counts['hello'], 2)
        self.assertEqual(counts['world'], 1)

    @patch('src.article_processor.requests.Session.get')
    def test_custom_base_url(self, mock_get):
        mock_response = MagicMock()
        mock_response.status_code = 200
        mock_response.text = "<html></html>"
        mock_get.return_value = mock_response

        custom_processor = ArticleProcessor(base_url="https://custom.wiki/")
        custom_processor.fetch_article("Term")

        args, _ = mock_get.call_args
        self.assertEqual(args[0], "https://custom.wiki/Term")


if __name__ == '__main__':
    unittest.main()
