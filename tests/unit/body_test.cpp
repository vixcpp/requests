/**
 *
 *  @file body_test.cpp
 *  @author Gaspard Kirira
 *
 *  @brief Unit tests for Vix Requests body helpers.
 *
 *  Copyright 2026, Gaspard Kirira.
 *  All rights reserved.
 *  https://github.com/vixcpp/requests
 *
 *  Use of this source code is governed by a MIT license
 *  that can be found in the LICENSE file.
 *
 *  Vix Requests
 *
 */

#include <vix/requests/Body.hpp>
#include <vix/requests/Params.hpp>

#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

namespace
{
  void expect(bool condition, const char *message)
  {
    if (!condition)
    {
      throw std::runtime_error(message);
    }
  }

  void test_empty_body()
  {
    const vix::requests::Body body;

    expect(body.type() == vix::requests::BodyType::Empty, "body type should be empty");
    expect(body.empty(), "body should be empty");
    expect(body.size() == 0, "body size should be zero");
    expect(body.data().empty(), "body data should be empty");
    expect(body.text().empty(), "body text should be empty");
    expect(body.bytes().empty(), "body bytes should be empty");
    expect(!body.has_content_type(), "empty body should not have content type");
  }

  void test_raw_body()
  {
    const auto body = vix::requests::raw_body(
        "hello",
        "text/plain");

    expect(body.type() == vix::requests::BodyType::Raw, "body type should be raw");
    expect(!body.empty(), "raw body should not be empty");
    expect(body.size() == 5, "raw body size should be 5");
    expect(body.data() == "hello", "raw body data should match");
    expect(body.text() == "hello", "raw body text should match");
    expect(body.content_type() == "text/plain", "raw body content type should match");
    expect(body.has_content_type(), "raw body should have content type");
  }

  void test_empty_raw_body_becomes_empty()
  {
    const auto body = vix::requests::raw_body("");

    expect(body.type() == vix::requests::BodyType::Empty,
           "empty raw body should become empty type");
    expect(body.empty(), "empty raw body should be empty");
  }

  void test_json_body()
  {
    const auto body = vix::requests::json_body(R"({"name":"Vix"})");

    expect(body.type() == vix::requests::BodyType::Json, "body type should be json");
    expect(body.data() == R"({"name":"Vix"})", "json body should match");
    expect(body.content_type() == "application/json", "json content type should match");
    expect(body.has_content_type(), "json body should have content type");
  }

  void test_form_body_from_params()
  {
    vix::requests::Params params;
    params.set("name", "Gaspard");
    params.set("project", "Vix Requests");
    params.set("symbol", "a+b&c=d");

    const auto body = vix::requests::form_body(params);

    expect(body.type() == vix::requests::BodyType::Form, "body type should be form");
    expect(
        body.data() == "name=Gaspard&project=Vix+Requests&symbol=a%2Bb%26c%3Dd",
        "form body should be form-url-encoded");
    expect(
        body.content_type() == "application/x-www-form-urlencoded",
        "form content type should match");
    expect(body.has_content_type(), "form body should have content type");
  }

  void test_form_body_from_initializer_list()
  {
    const auto body = vix::requests::form_body({{"name", "Gaspard"},
                                                {"project", "Vix"}});

    expect(body.type() == vix::requests::BodyType::Form, "body type should be form");
    expect(body.data() == "name=Gaspard&project=Vix", "form data should match");
    expect(
        body.content_type() == "application/x-www-form-urlencoded",
        "form content type should match");
  }

  void test_empty_form_body_becomes_empty()
  {
    const vix::requests::Params params;
    const auto body = vix::requests::form_body(params);

    expect(body.type() == vix::requests::BodyType::Empty,
           "empty form body should become empty type");
    expect(body.empty(), "empty form body should be empty");
    expect(body.content_type() == "application/x-www-form-urlencoded",
           "empty form body can still keep content type");
  }

  void test_binary_body()
  {
    std::vector<unsigned char> bytes{0x00U, 0x01U, 0x02U, 0xFFU};

    const auto body = vix::requests::binary_body(
        bytes,
        "application/octet-stream");

    expect(body.type() == vix::requests::BodyType::Binary, "body type should be binary");
    expect(body.size() == 4, "binary body size should be 4");
    expect(body.content_type() == "application/octet-stream",
           "binary content type should match");

    const auto restored = body.bytes();

    expect(restored.size() == bytes.size(), "restored bytes size should match");
    expect(restored[0] == 0x00U, "byte 0 should match");
    expect(restored[1] == 0x01U, "byte 1 should match");
    expect(restored[2] == 0x02U, "byte 2 should match");
    expect(restored[3] == 0xFFU, "byte 3 should match");
  }

  void test_body_type_to_string()
  {
    expect(vix::requests::to_string(vix::requests::BodyType::Empty) == "empty",
           "empty type string should match");
    expect(vix::requests::to_string(vix::requests::BodyType::Raw) == "raw",
           "raw type string should match");
    expect(vix::requests::to_string(vix::requests::BodyType::Json) == "json",
           "json type string should match");
    expect(vix::requests::to_string(vix::requests::BodyType::Form) == "form",
           "form type string should match");
    expect(vix::requests::to_string(vix::requests::BodyType::Binary) == "binary",
           "binary type string should match");
  }
}

int main()
{
  try
  {
    test_empty_body();
    test_raw_body();
    test_empty_raw_body_becomes_empty();
    test_json_body();
    test_form_body_from_params();
    test_form_body_from_initializer_list();
    test_empty_form_body_becomes_empty();
    test_binary_body();
    test_body_type_to_string();

    std::cout << "body_test passed\n";
    return 0;
  }
  catch (const std::exception &error)
  {
    std::cerr << "body_test failed: " << error.what() << '\n';
    return 1;
  }
}
