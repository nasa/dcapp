if __name__ == "__main__":
    print("<Dummy>\n")

    for xx in range(0, 1000, 10):
        for yy in range(0, 1000, 10):
            print(f"""\
<Constant Name="TriangleX">{xx}</Constant>
<Constant Name="TriangleY">{yy}</Constant>
<Include>triangle.xml</Include>
""")

    print("</Dummy>\n")
