import unittest
from unittest.mock import MagicMock, patch
import requests
from src.article_processor import ArticleProcessor, ArticleNotFound


class TestArticleProcessorErrors(unittest.TestCase):
    def setUp(self):
        self.processor = ArticleProcessor()

    @patch('src.article_processor.requests.Session.get')
    def test_fetch_article_404(self, mock_get):
        # Setup mock to raise HTTPError for 404
        mock_response = MagicMock()
        mock_response.status_code = 404
        mock_response.raise_for_status.side_effect = requests.exceptions.HTTPError(response=mock_response)
        mock_get.return_value = mock_response

        with self.assertRaises(ArticleNotFound) as cm:
            self.processor.fetch_article("NonExistentTerm")

        self.assertIn("not found", str(cm.exception))

    @patch('src.article_processor.requests.Session.get')
    def test_fetch_article_500(self, mock_get):
        # Setup mock to raise HTTPError for 500
        mock_response = MagicMock()
        mock_response.status_code = 500
        mock_response.raise_for_status.side_effect = requests.exceptions.HTTPError("Server Error",
                                                                                   response=mock_response)
        mock_get.return_value = mock_response

        with self.assertRaises(ConnectionError) as cm:
            self.processor.fetch_article("ServerFault")

        self.assertIn("HTTP error", str(cm.exception))


if __name__ == '__main__':
    unittest.main()
