from astroquery.mast import Catalogs
import os
import time

OUTPUT_PATH = 'inputs/star_ids.txt'  # Fix typo in file extension from .text to .txt
NUM_STARS_IDS = 2000

# Define query parameters
CATALOG = "TIC"

# Define filtering parameters for stars
QUERY_PARAMETERS = {
    "brightness_range": [0, 6]
}

# Query for stars
def query_star_catalog():
    """
    Query the MAST star catalog (e.g., TIC) for stars in the selected sector
    with optional filters like brightness.
    """
    try:
        print("Starting query...")
        start_time = time.time()  # Start the timer
        
        # Define the base query
        query_results = Catalogs.query_criteria(
            catalog=CATALOG,
            Tmag=(QUERY_PARAMETERS["brightness_range"][0], QUERY_PARAMETERS["brightness_range"][1])
        )
        
        # Calculate time taken
        query_duration = time.time() - start_time
        print(f"Query completed in {query_duration:.2f} seconds.")
        
        # Debug: Display a subset of the results
        if len(query_results) > 0:
            print(f"Sample of fetched star IDs: {query_results[:5]['ID']}")
        
        # Check if enough results were returned
        if len(query_results) < NUM_STARS_IDS:
            print(f"Warning: Only {len(query_results)} stars fetched (fewer than {NUM_STARS_IDS}).")
        else:
            print(f"Successfully fetched {len(query_results)} stars from {CATALOG}.")
        
        # Return the results as a table
        return query_results

    except Exception as e:
        print(f"Error querying the star catalog: {e}")
        return None

# Save star IDs to text file
def save_star_ids(query_results):
    try:
        print("Saving star IDs to file...")
        with open(OUTPUT_PATH, "w") as file:
            for i, row in enumerate(query_results[:NUM_STARS_IDS]):
                file.write(f"TIC {row['ID']}\n")
                # Print progress every 100 stars
                if i % 100 == 0:
                    print(f"Saved {i} star IDs...")

        print(f"Successfully saved {NUM_STARS_IDS} star IDs to {OUTPUT_PATH}.")

    except Exception as e:
        print(f"Error saving star IDs: {e}")

# Main function
if __name__ == "__main__":
    print("Fetching star IDs from MAST catalog...")
    results = query_star_catalog()

    if results is not None:
        print("Saving star IDs to file...")
        save_star_ids(results)
        print("Process complete.")
    else:
        print("Error fetching star IDs. Process incomplete.")
