    #include <gtest/gtest.h>
    #include "../AppInfrastructure/CsvSource.cpp"
    #include "../AppInfrastructure/CsvParser.cpp"
    #include "../AppInfrastructure/DataExtractor.cpp"

    // constants to help with test consistancy.
    std::string const csv_sample_line1 = "name,email\n";
    std::string const csv_sample_line2 = "Dijon,djmustard@spotty.com";
    std::string const csv_sample_col1  = "name";
    std::string const csv_sample_col2  = "email";
    std::string const csv_sample_cr1   = "Dijon";
    std::string const csv_sample_cr2   = "djmustard@spotty.com";
    int const csv_sample_num_rows      = 2;
    int const csv_sample_num_col       = 2;
    std::string const read_in_path     = "./test.csv";

    /**
     * helper function for testing CsvSource
     */
    auto setup_samp_csv_source_data() {
        // create a local file for an expirement
        std::ofstream out(read_in_path);
        out << csv_sample_line1 + csv_sample_line2;
        out.close();
        // create sample source obj
        CsvSource sample_source(read_in_path);

        return sample_source.read();
    }

    /**
     * helper function for testing CsvParser
     */
    auto setup_samp_csv_parse_data(){
        CsvParser sample_CSV_data(setup_samp_csv_source_data());
        return sample_CSV_data.parse();
    }

    TEST (CsvSource_test, checks_for_correct_file_reading) {
        // get sample raw data
        auto lines = setup_samp_csv_source_data();

        // check if the correct number of lines was parsed (should be 2)
        ASSERT_EQ(lines.size(), 2);

        // set up an iterator can check each line contents
        auto it = lines.begin();
        EXPECT_EQ(*it, csv_sample_line1.substr(0, csv_sample_line1.size() - 1));
        EXPECT_EQ(*++it, csv_sample_line2);
    }

    TEST (CsvParse_test, checks_for_correct_data_parsing) {
        // artificial raw data container (should be sourced from CsvSource)
        std::list<std::string> sample_data = {csv_sample_line1, csv_sample_line2};

        // get sample parsed data
        auto p_data = setup_samp_csv_parse_data();

        // check the size of the table (should only be one row)
        ASSERT_EQ(p_data.size(), 1);

        // check row contents
        EXPECT_EQ(p_data.front()[csv_sample_col1], csv_sample_cr1);
        EXPECT_EQ(p_data.back()[csv_sample_col2], csv_sample_cr2);
    }

    TEST (DataExtractor_test, checks_for_correct_CSV_data_extracting) {
        std::cout << std::endl << "if this test passes, then the above two test must also pass" << std::endl;

        // --- setup CsvSource and CsvParser---
        auto src = std::make_unique<CsvSource>(read_in_path);
        auto p = std::make_unique<CsvParser>();

        // --- Setting up DataExtractor ---
        DataExtractor test_extractor(std::move(src), std::move(p));

        auto sample_parsed_data = test_extractor.extract();

        // check the size of the ADT (should only be 1 given the sample data up top)
        ASSERT_EQ(sample_parsed_data.size(), 1);

        // check row contents
        auto& row = sample_parsed_data.front();
        EXPECT_EQ(row.at(csv_sample_col1), csv_sample_cr1);
        EXPECT_EQ(row.at(csv_sample_col2), csv_sample_cr2);
    }
