// gcc program.c -o program -I/usr/include/postgresql -L/usr/lib -lpq

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <libpq-fe.h>

typedef struct {
    int id;
    char travel_mode[10];
    double distance;
} Attendee;

typedef struct {
    int id;
    char activity_type[10];
} Activity;

double calculate_attendee_footprint(Attendee attendee) {
    if (strcmp(attendee.travel_mode, "car") == 0) {
        return attendee.distance * 0.24;
    } else if (strcmp(attendee.travel_mode, "plane") == 0) {
        return attendee.distance * 0.18;
    } else if (strcmp(attendee.travel_mode, "train") == 0) {
        return attendee.distance * 0.14;
    }
    return 0.0;
}

double calculate_activity_footprint(Activity activity) {
    return strcmp(activity.activity_type, "virtual") == 0 ? 10 : 50;
}

void calculate_and_update_event_footprint(PGconn *conn, int event_id) {
    double total_footprint = 0.0;
    PGresult *res;
    int i, nrows;

    // Calculate footprint for attendees
    char query[1024];
    snprintf(query, sizeof(query), "SELECT id, travel_mode, distance FROM attendees WHERE event_id = %d", event_id);
    res = PQexec(conn, query);
    if (PQresultStatus(res) != PGRES_TUPLES_OK) {
        fprintf(stderr, "SELECT attendees failed: %s\n", PQerrorMessage(conn));
        PQclear(res);
        return;
    }

    nrows = PQntuples(res);
    for (i = 0; i < nrows; i++) {
        Attendee attendee = {
            .id = atoi(PQgetvalue(res, i, 0)),
            .distance = atof(PQgetvalue(res, i, 2))
        };
        strncpy(attendee.travel_mode, PQgetvalue(res, i, 1), sizeof(attendee.travel_mode) - 1);
        total_footprint += calculate_attendee_footprint(attendee);
    }
    PQclear(res);

    // Calculate footprint for activities
    snprintf(query, sizeof(query), "SELECT id, activity_type FROM activities WHERE event_id = %d", event_id);
    res = PQexec(conn, query);
    if (PQresultStatus(res) != PGRES_TUPLES_OK) {
        fprintf(stderr, "SELECT activities failed: %s\n", PQerrorMessage(conn));
        PQclear(res);
        return;
    }

    nrows = PQntuples(res);
    for (i = 0; i < nrows; i++) {
        Activity activity = {
            .id = atoi(PQgetvalue(res, i, 0)),
        };
        strncpy(activity.activity_type, PQgetvalue(res, i, 1), sizeof(activity.activity_type) - 1);
        total_footprint += calculate_activity_footprint(activity);
    }
    PQclear(res);

    // Update event with the total carbon footprint
    snprintf(query, sizeof(query), "UPDATE events SET carbon_footprint = %f WHERE id = %d", total_footprint, event_id);
    res = PQexec(conn, query);
    if (PQresultStatus(res) != PGRES_COMMAND_OK) {
        fprintf(stderr, "UPDATE event failed: %s\n", PQerrorMessage(conn));
    }
    PQclear(res);
}

int main(int argc, char *argv[]) {
    // Connect to the database
    const char *conninfo = "dbname=your_database user=your_user password=your_password host=localhost";
    PGconn *conn = PQconnectdb(conninfo);

    if (PQstatus(conn) != CONNECTION_OK) {
        fprintf(stderr, "Connection to database failed: %s\n", PQerrorMessage(conn));
        PQfinish(conn);
        return 1;
    }

    int event_id = 1; // This would be dynamically determined or passed as a parameter
    calculate_and_update_event_footprint(conn, event_id);

    PQfinish(conn);
    return 0;
}
