import unittest
import sys
import os
from src.article_processor import ArticleProcessor


class TestUnitOffline(unittest.TestCase):
    """
    4 Unit tests for specific methods without internet connection.
    """

    def setUp(self):
        self.processor = ArticleProcessor(offline_path="data/test_integration")

    # Test 1: Link filtering logic
    def test_get_links_filtering(self):
        html = """
        <div class="mw-parser-output">
            <a href="/wiki/Valid_Link">Valid</a>
            <a href="/wiki/File:Image.png">Invalid (Namespace)</a>
            <a href="https://external.com">Invalid (External)</a>
            <a href="/wiki/Another_Valid">Valid 2</a>
        </div>
        """
        links = self.processor.get_links(html)
        self.assertIn("Valid_Link", links)
        self.assertIn("Another_Valid", links)
        self.assertNotIn("File:Image.png", links)
        self.assertNotIn("https://external.com", links)
        self.assertEqual(len(links), 2)

    # Test 2: Word counting (ignoring tags)
    def test_count_words_filtering(self):
        html = """
        <div class="mw-parser-output">
            <p>Word <script>ignore this</script> <b>BoldWord</b></p>
        </div>
        """
        counts = self.processor.count_words(html)
        self.assertEqual(counts.get("word"), 1)
        self.assertEqual(counts.get("boldword"), 1)
        self.assertNotIn("ignore", counts)
        self.assertNotIn("this", counts)

    # Test 3: Article fetching from local file (Offline Mode)
    def test_fetch_article_offline(self):
        # Relies on data/test_integration/Team_Rocket.html created previously
        html = self.processor.fetch_article("Team Rocket")
        self.assertIn("Rocket-dan", html)
        self.assertIn("Sevii Islands", html)

    # Test 4: Language confidence score calculation
    def test_lang_confidence_score(self):
        from src.wiki_scraper import WikiScraper
        scraper = WikiScraper()

        # Mock data
        # "the" is common, "le" is not in this language map
        article_counts = {"the": 10, "le": 5, "unknown": 1}
        lang_freqs = {"the": 1.0, "le": 0.0}  # "le" shouldn't happen if en, but let's assume it's not there

        # Only "the" contributes.
        # Total words = 16.
        # Score += (10/16) * 1.0 = 0.625

        score = scraper.lang_confidence_score(article_counts, {"the": 1.0})
        self.assertAlmostEqual(score, 10 / 16)


if __name__ == "__main__":
    unittest.main()
