import sys
import os
from src.wiki_scraper import WikiScraper


def run_integration_test():
    """
    Integration test that loads 'Team Rocket' from local file and verifies summary.
    Exits with code 1 on failure.
    """
    print("Running Integration Test: Offline Summary Extraction")

    # Path to test data
    data_path = os.path.abspath("data/test_integration")

    # Initialize Scraper in offline mode
    scraper = WikiScraper(offline_path=data_path)

    try:
        # Fetch and process
        # We manually call processor methods to verify return values directly,
        # mirroring what handle_summary does but allowing assertion.
        print(f"Fetching article 'Team Rocket' from {data_path}...")
        html = scraper.processor.fetch_article("Team Rocket")

        print("Extracting summary...")
        summary = scraper.processor.get_summary(html)

        print(f"Summary found:\n{summary}\n")

        # Assertions
        expected_start = "Team Rocket (Japanese: ロケット団 Rocket-dan, literally Rocket Gang) is a villainous team"
        expected_end = "outpost in the Sevii Islands."

        if not summary.startswith(expected_start):
            raise AssertionError(
                f"Summary does not start with expected text.\nExpected start: {expected_start}\nActual: {summary[:100]}...")

        if not summary.endswith(expected_end):
            raise AssertionError(
                f"Summary does not end with expected text.\nExpected end: {expected_end}\nActual: ...{summary[-50:]}")

        print("SUCCESS: Integration test passed.")
        sys.exit(0)

    except Exception as e:
        print(f"FAILURE: {e}")
        sys.exit(1)


if __name__ == "__main__":
    run_integration_test()
