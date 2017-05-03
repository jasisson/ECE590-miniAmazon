#include "add_cat.h"

void add_catalog(connection *C, string desc, int pid, string type) {
  string sql;
  
  sql = "INSERT INTO POLLS_CATALOG (CAT_DESC,CAT_PID,CAT_TYPE) "	\
    "VALUES ('" + desc + "'," + to_string(pid) + ",'" + type + "');";
  
  /* Create a transactional object. */
  work W(*C);
  
  /* Execute SQL query */
  W.exec( sql );
  W.commit(); 
}

void query1(connection *C,
	    int use_mpg, int min_mpg, int max_mpg,
            int use_ppg, int min_ppg, int max_ppg,
            int use_rpg, int min_rpg, int max_rpg,
            int use_apg, int min_apg, int max_apg,
            int use_spg, double min_spg, double max_spg,
            int use_bpg, double min_bpg, double max_bpg
            ) {

    string sql;
  
    sql = "SELECT * FROM player WHERE player_id = player_id";

    if (use_mpg == 1) {
      sql+= " AND mpg <= " + to_string(max_mpg) + "AND mpg >= " + to_string(min_mpg);
    }

    if (use_ppg == 1) {
      sql+= " AND ppg <= " + to_string(max_ppg) + "AND ppg >= " + to_string(min_ppg);
    }

    if (use_rpg == 1) {
      sql+= " AND rpg <= " + to_string(max_rpg) + "AND rpg >= " + to_string(min_rpg);
    }

    if (use_apg == 1) {
      sql+= " AND apg <= " + to_string(max_apg) + "AND apg >= " + to_string(min_apg);
    }

    if (use_spg == 1) {
      sql+= " AND spg <= " + to_string(max_spg) + "AND spg >= " + to_string(min_spg);
    }

    if (use_bpg == 1) {
      sql+= " AND bpg <= " + to_string(max_bpg) + "AND bpg >= " + to_string(min_bpg);
    }
  
  /* Create a non-transactional object. */
  nontransaction N(*C);

  /* Execute SQL query */
  result R( N.exec( sql ));

  /* List down all the records */
  cout << "PlayerID TeamID UniformNum FirstName LastName MPG PPG RPG APG SPG BPG" << endl;
  for (result::const_iterator c = R.begin(); c != R.end(); ++c) {
    cout <<c[0].as<string>()<<" "<<c[1].as<string>()<<" "<<c[2].as<string>()
	 <<" "<<c[3].as<string>()<<" "<<c[4].as<string>()<<" "<<c[5].as<string>()
	 <<" "<<c[6].as<string>()<<" "<<c[7].as<string>()<<" "<<c[8].as<string>()
	 <<" "<<c[9].as<string>()<<" "<<c[10].as<string>()<<" "<<endl;
  }
  //cout << "Operation done successfully" << endl;
}


void query2(connection *C, string team_color) {
  string sql;
  
  sql = "SELECT team.name FROM team, color WHERE color.color_id = team.color_id " \
    "AND color.name = '" + team_color + "'";
  
  /* Create a non-transactional object. */
  nontransaction N(*C);

  /* Execute SQL query */
  result R( N.exec( sql ));

  /* List down all the records */
  cout << "TeamName" << endl;
  for (result::const_iterator c = R.begin(); c != R.end(); ++c) {
    cout << c[0].as<string>() << endl;
  }
  //cout << "Operation done successfully" << endl;
}


void query3(connection *C, string team_name) {
  string sql;
  
  sql = "SELECT player.first_name, player.last_name FROM player, team WHERE player.team_id = "
    "team.team_id  AND team.name = '" + team_name + "' ORDER BY player.ppg DESC";
  
  /* Create a non-transactional object. */
  nontransaction N(*C);

  /* Execute SQL query */
  result R( N.exec( sql ));

  /* List down all the records */
  cout << "FirstName LastName" << endl;
  for (result::const_iterator c = R.begin(); c != R.end(); ++c) {
    cout << c[0].as<string>() << " " << c[1].as<string>() << endl;
  }
  //cout << "Operation done successfully" << endl;
}


void query4(connection *C, string team_state, string team_color) {
  string sql;
  
  sql = "SELECT player.first_name, player.last_name, player.uniform_num FROM player, team, state, color "
    "WHERE player.team_id = team.team_id AND state.state_id = team.state_id  AND team.color_id = "
    "color.color_id AND state.name = '" + team_state + "' AND color.name = '" + team_color + "'";
  
  /* Create a non-transactional object. */
  nontransaction N(*C);

  /* Execute SQL query */
  result R( N.exec( sql ));

  /* List down all the records */
  cout << "FirstName LastName JerseyNum" << endl;
  for (result::const_iterator c = R.begin(); c != R.end(); ++c) {
    cout << c[0].as<string>() << " " << c[1].as<string>() << " " << c[2].as<string>() << endl;
  }
  //cout << "Operation done successfully" << endl;
}


void query5(connection *C, int num_wins) {
   string sql;
  
  sql = "SELECT player.first_name, player.last_name, team.name, team.wins FROM player, team "
    "WHERE player.team_id = team.team_id AND team.wins > '" + to_string(num_wins) + "'";
  
  /* Create a non-transactional object. */
  nontransaction N(*C);

  /* Execute SQL query */
  result R( N.exec( sql ));

  /* List down all the records */
  cout << "FirstName LastName TeamName TeamWins" << endl;
  for (result::const_iterator c = R.begin(); c != R.end(); ++c) {
    cout<<c[0].as<string>()<<" "<<c[1].as<string>()<<" "<<c[2].as<string>()<<" "<<
      c[3].as<string>()<< endl;
  }
  //cout << "Operation done successfully" << endl;
}
