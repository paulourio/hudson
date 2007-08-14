/*
 * vix.cpp
 */

// Boost
#include <boost/program_options.hpp>

// Hudson
#include "YahooDriver.hpp"
#include "DaySeries.hpp"
#include "DayPrice.hpp"
#include "ReturnFactors.hpp"
#include "PositionFactors.hpp"
#include "VIXTrader.hpp"
#include "BnHTrader.hpp"
#include "Report.hpp"
#include "PositionsReport.hpp"

using namespace std;
using namespace boost::gregorian;
using namespace Series;

namespace po = boost::program_options;


int main(int argc, char* argv[])
{
  string begin_date, end_date;
  string dbfile, vix_dbfile;

  try {

    /*
    * Extract simulation options
    */
	  po::options_description desc("Allowed options");
	  desc.add_options()
	    ("help", "produce help message")
	    ("series_file",     po::value<string>(&dbfile),                       "series database")
      ("vix_series_file", po::value<string>(&vix_dbfile),                   "VIX series database")
	    ("begin_date",      po::value<string>(&begin_date),                   "start of trading period (YYYY-MM-DD)")
	    ("end_date",        po::value<string>(&end_date),                     "end of trading period (YYYY-MM-DD)")
	    ;

	  po::variables_map vm;
	  po::store(po::parse_command_line(argc, argv, desc), vm);
	  po::notify(vm);

	  if( vm.count("help") ) {
	    cout << desc << endl;
	    exit(0);
	  }

	  if( vm["series_file"].empty() || vm["vix_series_file"].empty() ||
		    vm["begin_date"].empty() || vm["end_date"].empty() ) {
	    cout << desc << endl;
	    exit(1);
	  }

	  cout << "Series file: " << dbfile << endl;
    cout << "VIX series file: " << vix_dbfile << endl;

    /*
    * Load series data
    */
    YahooDriver yd;
    DaySeries db("prices", yd);
    DaySeries vixdb("vix", yd);

	  date load_begin(from_simple_string(begin_date));
	  if( load_begin.is_not_a_date() ) {
	    cerr << "Invalid begin date " << begin_date << endl;
	    exit(EXIT_FAILURE);
	  }

	  date load_end(from_simple_string(end_date));
	  if( load_end.is_not_a_date() ) {
	    cerr << "Invalid end date " << end_date << endl;
	    exit(EXIT_FAILURE);
	  }

	  cout << "Loading " << dbfile << " from " << to_simple_string(load_begin) << " to " << to_simple_string(load_end) << "..." << endl;
	  if( db.load(dbfile, load_begin, load_end) <= 0 ) {
	    cerr << "No records found" << endl;
	    exit(EXIT_FAILURE);
	  }

    cout << "Loading " << vix_dbfile << " from " << to_simple_string(load_begin) << " to " << to_simple_string(load_end) << "..." << endl;
    if( vixdb.load(vix_dbfile, load_begin, load_end) <= 0 ) {
      cerr << "No records found" << endl;
      exit(EXIT_FAILURE);
    }

    cout << "Records: " << db.size() << endl;
    cout << "Period: " << db.period() << endl;
    cout << "Total days: " << db.duration().days() << endl;

    /*
    * Initialize and run strategy
    */
    VIXTrader trader(db, vixdb);
    trader.run();
    trader.positions().closed().print();
    cout << "Invested days: " << trader.invested_days() << " (" << (trader.invested_days().days()/(double)db.duration().days()) * 100 << "%)" << endl;

    /*
    * Print simulation reports
    */
    ReturnFactors rf(trader.positions().closed(), db.duration().days(), 12);
    PositionFactorsSet pf(trader.positions().closed(), db);

    Report rp(rf);
    rp.print();

    // Positions excursion
    cout << endl << "Position Excursions" << endl << "--" << endl;
    PositionsReport pr(pf);
    pr.print();

  } catch( std::exception& ex ) {

	  cerr << ex.what() << endl;
	  exit(EXIT_FAILURE);
  }

  return 0;
}