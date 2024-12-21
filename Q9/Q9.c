#include <gtk/gtk.h>

// 콜백 함수: 연산 결과 계산
void on_calculate_clicked(GtkWidget *button, gpointer data) {
    // 데이터 구조체 가져오기
    GtkWidget **widgets = (GtkWidget **)data;
    GtkWidget *entry1 = widgets[0];
    GtkWidget *entry2 = widgets[1];
    GtkWidget *label_result = widgets[2];

    // 사용자 입력 가져오기
    const char *num1_text = gtk_entry_get_text(GTK_ENTRY(entry1));
    const char *num2_text = gtk_entry_get_text(GTK_ENTRY(entry2));
    double num1 = atof(num1_text);
    double num2 = atof(num2_text);

    // 버튼 라벨 가져오기
    const char *operation = gtk_button_get_label(GTK_BUTTON(button));
    double result = 0;

    // 연산 수행
    if (strcmp(operation, "+") == 0) {
        result = num1 + num2;
    } else if (strcmp(operation, "-") == 0) {
        result = num1 - num2;
    } else if (strcmp(operation, "*") == 0) {
        result = num1 * num2;
    } else if (strcmp(operation, "/") == 0) {
        if (num2 != 0) {
            result = num1 / num2;
        } else {
            gtk_label_set_text(GTK_LABEL(label_result), "Error: Division by zero");
            return;
        }
    }

    // 결과 표시
    char result_text[50];
    snprintf(result_text, sizeof(result_text), "Result: %.2f", result);
    gtk_label_set_text(GTK_LABEL(label_result), result_text);
}

int main(int argc, char *argv[]) {
    GtkWidget *window;
    GtkWidget *grid;
    GtkWidget *entry1, *entry2, *label_result;
    GtkWidget *button_add, *button_sub, *button_mul, *button_div;

    gtk_init(&argc, &argv);

    // 윈도우 생성
    window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_window_set_title(GTK_WINDOW(window), "GTK+ Calculator");
    gtk_window_set_default_size(GTK_WINDOW(window), 300, 200);
    g_signal_connect(window, "destroy", G_CALLBACK(gtk_main_quit), NULL);

    // 그리드 레이아웃 생성
    grid = gtk_grid_new();
    gtk_container_add(GTK_CONTAINER(window), grid);

    // 입력 필드 및 라벨 추가
    entry1 = gtk_entry_new();
    entry2 = gtk_entry_new();
    gtk_grid_attach(GTK_GRID(grid), gtk_label_new("Number 1:"), 0, 0, 1, 1);
    gtk_grid_attach(GTK_GRID(grid), entry1, 1, 0, 2, 1);
    gtk_grid_attach(GTK_GRID(grid), gtk_label_new("Number 2:"), 0, 1, 1, 1);
    gtk_grid_attach(GTK_GRID(grid), entry2, 1, 1, 2, 1);

    // 결과 라벨 추가
    label_result = gtk_label_new("Result: ");
    gtk_grid_attach(GTK_GRID(grid), label_result, 0, 2, 3, 1);

    // 연산 버튼 추가
    button_add = gtk_button_new_with_label("+");
    button_sub = gtk_button_new_with_label("-");
    button_mul = gtk_button_new_with_label("*");
    button_div = gtk_button_new_with_label("/");

    gtk_grid_attach(GTK_GRID(grid), button_add, 0, 3, 1, 1);
    gtk_grid_attach(GTK_GRID(grid), button_sub, 1, 3, 1, 1);
    gtk_grid_attach(GTK_GRID(grid), button_mul, 0, 4, 1, 1);
    gtk_grid_attach(GTK_GRID(grid), button_div, 1, 4, 1, 1);

    // 버튼 클릭 시 콜백 함수 연결
    GtkWidget *widgets[] = {entry1, entry2, label_result};
    g_signal_connect(button_add, "clicked", G_CALLBACK(on_calculate_clicked), widgets);
    g_signal_connect(button_sub, "clicked", G_CALLBACK(on_calculate_clicked), widgets);
    g_signal_connect(button_mul, "clicked", G_CALLBACK(on_calculate_clicked), widgets);
    g_signal_connect(button_div, "clicked", G_CALLBACK(on_calculate_clicked), widgets);

    // 창 표시
    gtk_widget_show_all(window);
    gtk_main();

    return 0;
}
