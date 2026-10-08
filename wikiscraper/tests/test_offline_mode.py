import unittest
import os
import shutil
from src.wiki_scraper import WikiScraper
from src.article_processor import ArticleNotFound


class TestOfflineMode(unittest.TestCase):
    def setUp(self):
        self.test_dir = "test_offline_data"
        os.makedirs(self.test_dir, exist_ok=True)

        # Create a dummy article
        with open(os.path.join(self.test_dir, "Pikachu.html"), "w") as f:
            f.write("<html><body><div class='mw-parser-output'><p>Pikachu is electric.</p></div></body></html>")

    def tearDown(self):
        shutil.rmtree(self.test_dir)

    def test_offline_fetch_success(self):
        scraper = WikiScraper(offline_path=self.test_dir)

        # Capture output? Or just check internal processor result
        html = scraper.processor.fetch_article("Pikachu")
        self.assertIn("Pikachu is electric", html)

    def test_offline_fetch_not_found(self):
        scraper = WikiScraper(offline_path=self.test_dir)

        with self.assertRaises(ArticleNotFound):
            scraper.processor.fetch_article("MissingMon")

    def test_scraper_integration_offline(self):
        # Verify handle_summary works with offline data
        scraper = WikiScraper(offline_path=self.test_dir)

        # Mock print to avoid clutter
        from unittest.mock import patch
        with patch('builtins.print') as mock_print:
            scraper.handle_summary("Pikachu")
            # Verify output contains summary
            # print call args: (f"Summary...",)
            output = mock_print.call_args[0][0]
            self.assertIn("Pikachu is electric", output)


if __name__ == '__main__':
    unittest.main()
