package ar.fiuba.jobify;

import android.content.Intent;
import android.content.SharedPreferences;
import android.net.Uri;
import android.support.design.widget.NavigationView;
import android.support.v4.view.GravityCompat;
import android.support.v4.widget.DrawerLayout;
import android.support.v7.app.ActionBarDrawerToggle;
import android.support.v7.app.AppCompatActivity;
import android.support.v7.widget.Toolbar;
import android.util.Log;
import android.view.Menu;
import android.view.MenuItem;
import android.view.View;
import android.widget.ImageView;
import android.widget.LinearLayout;
import android.widget.TextView;

import com.android.volley.ParseError;
import com.android.volley.Request;
import com.android.volley.Response;
import com.android.volley.VolleyError;
import com.android.volley.toolbox.JsonObjectRequest;

import org.json.JSONException;
import org.json.JSONObject;

import ar.fiuba.jobify.app_server_api.User;

public class NavDrawerActivity extends AppCompatActivity
        implements NavigationView.OnNavigationItemSelectedListener {

    private final String LOG_TAG = NavDrawerActivity.class.getSimpleName();

    public long connectedUserID = 0;


    protected void onCreateDrawer() {
        Toolbar toolbar = (Toolbar) findViewById(R.id.perfil_toolbar);
        setSupportActionBar(toolbar);

        SharedPreferences sharedPref =
                getSharedPreferences(getString(R.string.shared_pref_connected_user), 0);
        connectedUserID = sharedPref
                .getLong(getString(R.string.stored_connected_user_id), connectedUserID);

        DrawerLayout drawer = (DrawerLayout) findViewById(R.id.drawer_layout);
        ActionBarDrawerToggle toggle = new ActionBarDrawerToggle(
                this, drawer, toolbar, R.string.navigation_drawer_open, R.string.navigation_drawer_close) {
//            public void onDrawerOpened() {}
//            public void onDrawerClosed() {}
        };
        if (drawer != null) drawer.addDrawerListener(toggle);
        toggle.syncState();

        NavigationView navigationView = (NavigationView) findViewById(R.id.nav_view);
        if (navigationView != null) navigationView.setNavigationItemSelectedListener(this);

        setUpDrawerHeader();
    }

    private void setUpDrawerHeader() {
        setUpDrawerHeaderUser();

        LinearLayout headerLayout = (LinearLayout) findViewById(R.id.nav_drawer_header_layout);
        if (headerLayout != null) {
            headerLayout.setOnClickListener(new View.OnClickListener() {
                @Override
                public void onClick(View v) {
                    if (connectedUserID != 0) {
                        startActivity(
                                new Intent(NavDrawerActivity.this, PerfilActivity.class)
                                        .putExtra(PerfilActivity.FETCHED_USER_ID_MESSAGE, connectedUserID)
                        );
                    }
                }
            });
        }
    }

    private void fillDrawerHeaderText(User user) {
        TextView tv_nombre = (TextView) findViewById(R.id.nav_drawer_nombre);
        if (tv_nombre != null)
            tv_nombre.setText(user.getFullName());

        TextView tv_mail = (TextView) findViewById(R.id.nav_drawer_mail);
        if (tv_mail != null)
            tv_mail.setText(user.getEmail());
    }

    @Override
    public void onBackPressed() {
        DrawerLayout drawer = (DrawerLayout) findViewById(R.id.drawer_layout);
        if (drawer != null && drawer.isDrawerOpen(GravityCompat.START)) {
            drawer.closeDrawer(GravityCompat.START);
        } else {
            super.onBackPressed();
        }
    }

    @Override
    public boolean onCreateOptionsMenu(Menu menu) {
        // Inflate the menu; this adds items to the action bar if it is present.
        getMenuInflater().inflate(R.menu.nav_drawer, menu);
        return true;
    }

    @Override
    public boolean onOptionsItemSelected(MenuItem item) {
        // Handle action bar item clicks here. The action bar will
        // automatically handle clicks on the Home/Up button, so long
        // as you specify a parent activity in AndroidManifest.xml.
        int id = item.getItemId();

        //noinspection SimplifiableIfStatement
        if (id == R.id.action_settings) {
            startActivity(new Intent(this, SettingsActivity.class));
            return true;
        }

        return super.onOptionsItemSelected(item);
    }

    @SuppressWarnings("StatementWithEmptyBody")
    @Override
    public boolean onNavigationItemSelected(MenuItem item) {
        // Handle navigation view item clicks here.
        int id = item.getItemId();

        if (id == R.id.nav_manage) {
            startActivity(new Intent(this, SettingsActivity.class));
            return true;
        }
//        } else if (id == R.id.nav_gallery) {

        DrawerLayout drawer = (DrawerLayout) findViewById(R.id.drawer_layout);
        if (drawer != null)
            drawer.closeDrawer(GravityCompat.START);
        return true;
    }

    @Override
    protected void onResume() {
        super.onResume();
        DrawerLayout drawer = (DrawerLayout) findViewById(R.id.drawer_layout);
        if (drawer != null)
            drawer.closeDrawer(GravityCompat.START);
    }

    public void setUpDrawerHeaderUser() {

        Uri builtUri = Uri.parse(Utils.getAppServerBaseURL()).buildUpon()
                .appendPath(getString(R.string.perfil_get_user_path))
                .appendPath(Long.toString(connectedUserID))
                .build();
        final String urlGetUser = builtUri.toString();

        JsonObjectRequest jsObjRequest = new JsonObjectRequest
                (Request.Method.GET, urlGetUser, null, new Response.Listener<JSONObject>() {

                    @Override
                    public void onResponse(JSONObject response) {
                        User mUser = User.parseJSON(response.toString());

                        if (mUser != null) {
                            fillDrawerHeaderText(mUser);
                        } else {
                            Log.e(LOG_TAG, "Error de parseo de usuario, no puedo llenar el header del ND");
                        }
                    }

                }, new Response.ErrorListener() {

                    @Override
                    public void onErrorResponse(VolleyError error) {
                        Log.d(LOG_TAG, "errortostring "+error.toString());
                        Log.d(LOG_TAG, "urrrrrrrrl: "+urlGetUser);//
                        if (error instanceof ParseError && error.getCause() instanceof JSONException) {
                            Log.d(LOG_TAG, "JSONException! Intento refreshear de nuevo...");
                            setUpDrawerHeaderUser();
                        }
                    }
                });
        jsObjRequest.setTag(LOG_TAG);

        RequestQueueSingleton.getInstance(this.getApplicationContext())
                .addToRequestQueue(jsObjRequest);


        ImageView iv_thumbnail = (ImageView) findViewById(R.id.nav_drawer_thumbnail);
        builtUri = Uri.parse(Utils.getAppServerBaseURL()).buildUpon()
                .appendPath(getString(R.string.perfil_get_thumbnail_path))
                .appendPath(Long.toString(connectedUserID))
                .build();
        String urlGetThumbnail = builtUri.toString();

        Utils.cargarImagenDeURLenImageView(this, iv_thumbnail, urlGetThumbnail, LOG_TAG);
    }
}